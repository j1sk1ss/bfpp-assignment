use crate::CompileError;

/// BF ignores extensions; BF++ enables labels, calls and slash comments.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum Dialect {
    Bf,
    #[default]
    Bfpp,
}

/// Typed, resolved bytecode. Jump operands are instruction indices.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Instruction {
    Add(i32),
    Move(i32),
    Input,
    Output,
    JumpIfZero(usize),
    JumpIfNonzero(usize),
    Zero,
    Jump(usize),
    Call(usize),
    Return,
    LoadArgument(u8),
    StoreArgument(u8),
    LoadReturn,
}

/// Immutable code and label metadata, independent of runtime call frames.
#[derive(Debug, Clone)]
pub struct Program {
    pub(crate) code: Vec<Instruction>,
    labels: [Option<usize>; 26],
}

impl Program {
    /// Compile raw source bytes, resolving forward labels and matching loops.
    /// Repeated label definitions keep the first definition, as in the C tool.
    pub fn compile(source: &[u8], dialect: Dialect) -> Result<Self, CompileError> {
        let mut program = Self {
            code: Vec::new(),
            labels: [None; 26],
        };
        let mut loops = Vec::new();
        let mut patches = Vec::new();
        let mut comment = false;
        let mut i = 0;
        let extensions = dialect == Dialect::Bfpp;
        let error = |offset, message: String| CompileError { offset, message };

        while i < source.len() {
            let c = source[i];
            if extensions && c == b'/' {
                comment = !comment;
                i += 1;
                continue;
            }
            if comment {
                i += 1;
                continue;
            }
            if matches!(c, b'+' | b'-' | b'>' | b'<') {
                let arithmetic = matches!(c, b'+' | b'-');
                let mut amount = 0i64;
                while i < source.len() {
                    let delta = match (arithmetic, source[i]) {
                        (true, b'+') | (false, b'>') => 1,
                        (true, b'-') | (false, b'<') => -1,
                        _ => break,
                    };
                    amount += delta;
                    i += 1;
                }
                // Preserve run folding and i32 chunking, including move bounds
                // checks on the net displacement rather than each character.
                while amount != 0 {
                    let chunk = amount.clamp(i32::MIN as i64, i32::MAX as i64) as i32;
                    program.code.push(if arithmetic {
                        Instruction::Add(chunk)
                    } else {
                        Instruction::Move(chunk)
                    });
                    amount -= i64::from(chunk);
                }
                continue;
            }
            match c {
                b',' => program.code.push(Instruction::Input),
                b'.' => program.code.push(Instruction::Output),
                b'[' => {
                    loops.push(program.code.len());
                    program.code.push(Instruction::JumpIfZero(0));
                }
                b']' => {
                    let open = loops.pop().ok_or_else(|| {
                        error(i, format!("unmatched closing bracket at byte {i}"))
                    })?;
                    program.code.push(Instruction::JumpIfNonzero(open + 1));
                    program.code[open] = Instruction::JumpIfZero(program.code.len());
                }
                b'*' if extensions => program.code.push(Instruction::Zero),
                b'!' if extensions => program.code.push(Instruction::Return),
                b'$' if extensions => program.code.push(Instruction::LoadReturn),
                b'^' | b'&' if extensions => {
                    let label = source
                        .get(i + 1)
                        .copied()
                        .filter(u8::is_ascii_lowercase)
                        .ok_or_else(|| {
                            error(
                                i,
                                format!("'{}' must be followed by a label a-z", char::from(c)),
                            )
                        })?;
                    patches.push((program.code.len(), label));
                    program.code.push(if c == b'^' {
                        Instruction::Jump(0)
                    } else {
                        Instruction::Call(0)
                    });
                    i += 1;
                }
                b'#' if extensions => {
                    let argument = source
                        .get(i + 1)
                        .copied()
                        .filter(u8::is_ascii_digit)
                        .ok_or_else(|| {
                            error(i, "'#' must be followed by an argument index 0-9".into())
                        })?;
                    program
                        .code
                        .push(Instruction::LoadArgument(argument - b'0'));
                    i += 1;
                }
                b'0'..=b'9' if extensions => {
                    program.code.push(Instruction::StoreArgument(c - b'0'))
                }
                b'a'..=b'z' if extensions => {
                    program.labels[usize::from(c - b'a')].get_or_insert(program.code.len());
                }
                _ => {}
            }
            i += 1;
        }
        if comment {
            return Err(error(source.len(), "unterminated '/' comment".into()));
        }
        if !loops.is_empty() {
            return Err(error(source.len(), "unmatched opening bracket".into()));
        }
        for (index, label) in patches {
            let target = program
                .label(label)
                .ok_or_else(|| error(0, format!("undefined label '{}'", char::from(label))))?;
            program.code[index] = match program.code[index] {
                Instruction::Jump(_) => Instruction::Jump(target),
                _ => Instruction::Call(target),
            };
        }
        Ok(program)
    }

    /// Inspect compiled instructions without mutating validated bytecode.
    pub fn instructions(&self) -> &[Instruction] {
        &self.code
    }

    /// Look up a lowercase ASCII label, including labels at the end of code.
    pub fn label(&self, label: u8) -> Option<usize> {
        if label.is_ascii_lowercase() {
            self.labels[usize::from(label - b'a')]
        } else {
            None
        }
    }
}
