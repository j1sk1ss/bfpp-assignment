use std::{fs, io};

use bfpp::{Dialect, Instruction, Program, RuntimeErrorKind, TapeKind, VmConfig};

fn run(source: &[u8], config: VmConfig) -> Result<Vec<u8>, bfpp::RuntimeError> {
    let program = Program::compile(source, Dialect::Bfpp).unwrap();
    let mut output = Vec::new();
    program.run(&mut io::empty(), &mut output, config)?;
    Ok(output)
}

#[test]
fn existing_compiler_fixtures_match_original_bytecode_and_offsets() {
    let cases = include_str!("fixtures/compiler/cases.txt");
    for case in cases.lines() {
        let fields: Vec<_> = case.split_whitespace().collect();
        let source = fs::read(format!("tests/fixtures/{}", fields[0])).unwrap();
        let dialect = if fields[2] == "0" {
            Dialect::Bf
        } else {
            Dialect::Bfpp
        };
        let result = Program::compile(&source, dialect);
        if fields[3] != "0" {
            assert_eq!(
                result.unwrap_err().offset,
                fields[4].parse::<usize>().unwrap(),
                "{case}"
            );
            continue;
        }
        let expected = fs::read_to_string(format!("tests/fixtures/{}", fields[1])).unwrap();
        let expected: Vec<_> = expected
            .lines()
            .map(|line| {
                let mut words = line.split_whitespace();
                let opcode = words.next().unwrap().parse::<u8>().unwrap();
                let operand = words.next().unwrap().parse::<i32>().unwrap();
                (opcode, operand)
            })
            .collect();
        let actual: Vec<_> = result
            .unwrap()
            .instructions()
            .iter()
            .map(|instruction| match *instruction {
                Instruction::Add(n) => (0, n),
                Instruction::Move(n) => (1, n),
                Instruction::Input => (2, 0),
                Instruction::Output => (3, 0),
                Instruction::JumpIfZero(n) => (4, n as i32),
                Instruction::JumpIfNonzero(n) => (5, n as i32),
                Instruction::Zero => (6, 0),
                Instruction::Jump(n) => (7, n as i32),
                Instruction::Call(n) => (8, n as i32),
                Instruction::Return => (9, 0),
                Instruction::LoadArgument(n) => (10, i32::from(n)),
                Instruction::StoreArgument(n) => (11, i32::from(n)),
                Instruction::LoadReturn => (12, 0),
            })
            .collect();
        assert_eq!(actual, expected, "{case}");
    }
}

#[test]
fn syntax_errors_use_byte_offsets_and_require_adjacent_operands() {
    for (source, offset) in [
        (&b"\xff]"[..], 1),
        (&b"["[..], 1),
        (&b"/abc"[..], 4),
        (&b"^ a"[..], 0),
        (&b"&A"[..], 0),
        (&b"# 0"[..], 0),
        (&b"#x"[..], 0),
        (&b"&z"[..], 0),
    ] {
        assert_eq!(
            Program::compile(source, Dialect::Bfpp).unwrap_err().offset,
            offset
        );
    }
}

#[test]
fn labels_remain_available_separately_and_first_definition_wins() {
    let program = Program::compile(b"^a+a++!z", Dialect::Bfpp).unwrap();
    assert_eq!(program.label(b'a'), Some(2));
    assert_eq!(program.label(b'z'), Some(program.instructions().len()));
    assert_eq!(program.label(b'A'), None);
    assert_eq!(program.label(b'x'), None);
    assert_eq!(run(b"^a+a++!z", VmConfig::default()).unwrap(), []);
}

#[test]
fn opposite_moves_cancel_before_bounds_check_but_whitespace_splits_runs() {
    assert_eq!(run(b"<>+.", VmConfig::default()).unwrap(), [1]);
    let error = run(b"< >", VmConfig::default()).unwrap_err();
    assert_eq!((error.kind, error.pc), (RuntimeErrorKind::TapeBounds, 0));
}

#[test]
fn tape_limits_and_zero_initialized_growth() {
    for (kind, limit) in [(TapeKind::Fixed, 30_000), (TapeKind::Grow, 1_048_576)] {
        let config = VmConfig {
            tape: kind,
            ..VmConfig::default()
        };
        let mut source = vec![b'>'; limit - 1];
        source.push(b'.');
        assert_eq!(run(&source, config).unwrap(), [0]);
        source.push(b'>');
        let error = run(&source, config).unwrap_err();
        assert_eq!((error.kind, error.pc), (RuntimeErrorKind::TapeBounds, 2));
    }
}

#[test]
fn main_argument_load_is_noop_and_reused_callee_frames_are_fresh() {
    assert_eq!(run(b"++#0.", VmConfig::default()).unwrap(), [2]);
    // f sets its outgoing arg and last return; its next invocation starts clean.
    assert_eq!(
        run(b"&f&f!f#0.$.+0&g!g+!", VmConfig::default()).unwrap(),
        [0, 0, 0, 0]
    );
}

#[test]
fn reaching_code_end_inside_function_halts_without_implicit_return() {
    assert_eq!(run(b"&f+.!f++", VmConfig::default()).unwrap(), []);
}

#[test]
fn recursive_calls_and_infinite_loops_have_configurable_limits() {
    let error = run(
        b"f&f",
        VmConfig {
            max_call_depth: 3,
            ..VmConfig::default()
        },
    )
    .unwrap_err();
    assert_eq!((error.kind, error.pc), (RuntimeErrorKind::CallDepth, 0));
    let error = run(
        b"+[]",
        VmConfig {
            max_steps: Some(10),
            ..VmConfig::default()
        },
    )
    .unwrap_err();
    assert_eq!((error.kind, error.pc), (RuntimeErrorKind::StepLimit, 2));
}

#[test]
fn compiled_program_can_be_reused_with_fresh_state_and_binary_streams() {
    let program = Program::compile(b",.,.,.", Dialect::Bf).unwrap();
    for _ in 0..2 {
        let mut output = Vec::new();
        assert_eq!(
            program
                .run(&mut &b"\x00\xff"[..], &mut output, VmConfig::default())
                .unwrap(),
            6
        );
        assert_eq!(output, [0, 255, 0]);
    }
}

struct FailingOutput {
    fail_flush: bool,
}

impl io::Write for FailingOutput {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        if self.fail_flush {
            Ok(bytes.len())
        } else {
            Err(io::Error::other("write failure"))
        }
    }
    fn flush(&mut self) -> io::Result<()> {
        Err(io::Error::other("flush failure"))
    }
}

#[test]
fn reports_write_and_flush_failures_at_instruction_indices() {
    let program = Program::compile(b".!", Dialect::Bfpp).unwrap();
    for (fail_flush, expected_pc) in [(false, 0), (true, 2)] {
        let error = program
            .run(
                &mut io::empty(),
                &mut FailingOutput { fail_flush },
                VmConfig::default(),
            )
            .unwrap_err();
        assert_eq!((error.kind, error.pc), (RuntimeErrorKind::Io, expected_pc));
    }
}
