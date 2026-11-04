use std::io::{self, Read, Write};

use crate::tape::Tape;
use crate::{Instruction, Program, RuntimeError, RuntimeErrorKind, TapeKind};

/// Execution limits. Call depth includes the main program's frame.
#[derive(Debug, Clone, Copy)]
pub struct VmConfig {
    pub tape: TapeKind,
    pub max_call_depth: usize,
    /// Maximum executed bytecode instructions; `None` means unlimited.
    pub max_steps: Option<u64>,
}

impl Default for VmConfig {
    fn default() -> Self {
        Self {
            tape: TapeKind::Grow,
            max_call_depth: 128,
            max_steps: None,
        }
    }
}

struct Frame {
    tape: Tape,
    return_pc: usize,
    arguments: [u8; 10],
    last_return: u8,
}

impl Frame {
    fn new(tape: TapeKind, return_pc: usize) -> Self {
        Self {
            tape: Tape::new(tape),
            return_pc,
            arguments: [0; 10],
            last_return: 0,
        }
    }
}

impl Program {
    /// Execute with fresh state and byte-oriented I/O, returning the step count.
    /// EOF (and input errors, for compatibility with C) writes zero. Output and
    /// flush failures are reported. Reaching the end of code halts all frames.
    pub fn run<R: Read, W: Write>(
        &self,
        input: &mut R,
        output: &mut W,
        config: VmConfig,
    ) -> Result<u64, RuntimeError> {
        let mut frames = vec![Frame::new(config.tape, 0)];
        let mut pc = 0;
        let mut steps = 0;
        // The C API uses a fallback of 64 when explicitly configured with zero.
        let max_depth = if config.max_call_depth == 0 {
            64
        } else {
            config.max_call_depth
        };
        while let Some(&instruction) = self.code.get(pc) {
            if config.max_steps.is_some_and(|limit| steps >= limit) {
                return Err(RuntimeError::new(
                    RuntimeErrorKind::StepLimit,
                    pc,
                    "step limit exceeded",
                ));
            }
            steps += 1;
            let depth = frames.len();
            let frame = &mut frames[depth - 1];
            match instruction {
                Instruction::Add(delta) => {
                    let cell = frame.tape.cell();
                    *cell = cell.wrapping_add(delta as u8);
                }
                Instruction::Move(delta) => frame.tape.move_by(delta).map_err(|()| {
                    RuntimeError::new(
                        RuntimeErrorKind::TapeBounds,
                        pc,
                        format!("tape move out of bounds at pc={pc}"),
                    )
                })?,
                Instruction::Input => {
                    let mut byte = [0];
                    loop {
                        match input.read(&mut byte) {
                            Ok(1) => break,
                            Err(e) if e.kind() == io::ErrorKind::Interrupted => continue,
                            _ => {
                                byte[0] = 0;
                                break;
                            }
                        }
                    }
                    *frame.tape.cell() = byte[0];
                }
                Instruction::Output => output.write_all(&[*frame.tape.cell()]).map_err(|e| {
                    RuntimeError::new(
                        RuntimeErrorKind::Io,
                        pc,
                        format!("output error at pc={pc}: {e}"),
                    )
                })?,
                Instruction::JumpIfZero(target) if *frame.tape.cell() == 0 => {
                    pc = target;
                    continue;
                }
                Instruction::JumpIfNonzero(target) if *frame.tape.cell() != 0 => {
                    pc = target;
                    continue;
                }
                Instruction::Jump(target) => {
                    pc = target;
                    continue;
                }
                Instruction::Zero => *frame.tape.cell() = 0,
                Instruction::Call(target) => {
                    if depth >= max_depth {
                        return Err(RuntimeError::new(
                            RuntimeErrorKind::CallDepth,
                            pc,
                            format!("maximum call depth ({max_depth}) exceeded"),
                        ));
                    }
                    frames.push(Frame::new(config.tape, pc + 1));
                    pc = target;
                    continue;
                }
                Instruction::Return => {
                    if depth == 1 {
                        break;
                    }
                    let value = *frame.tape.cell();
                    pc = frame.return_pc;
                    frames.pop();
                    frames[depth - 2].last_return = value;
                    continue;
                }
                Instruction::LoadArgument(argument) => {
                    if depth > 1 {
                        let value = frames[depth - 2].arguments[usize::from(argument)];
                        *frames[depth - 1].tape.cell() = value;
                    }
                }
                Instruction::StoreArgument(argument) => {
                    frame.arguments[usize::from(argument)] = *frame.tape.cell()
                }
                Instruction::LoadReturn => *frame.tape.cell() = frame.last_return,
                Instruction::JumpIfZero(_) | Instruction::JumpIfNonzero(_) => {}
            }
            pc += 1;
        }
        output.flush().map_err(|e| {
            RuntimeError::new(
                RuntimeErrorKind::Io,
                self.code.len(),
                format!("flush failed: {e}"),
            )
        })?;
        Ok(steps)
    }
}
