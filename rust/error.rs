use std::fmt;

/// A syntax error located at a byte offset in the source.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CompileError {
    pub offset: usize,
    pub message: String,
}

impl fmt::Display for CompileError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "compile error [bf_err_syntax] at {}: {}",
            self.offset, self.message
        )
    }
}

impl std::error::Error for CompileError {}

/// Categories of execution failures, retaining the original diagnostic names.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RuntimeErrorKind {
    TapeBounds,
    CallDepth,
    StepLimit,
    Io,
}

impl RuntimeErrorKind {
    fn status(self) -> &'static str {
        match self {
            Self::TapeBounds => "bf_err_tape_bounds",
            Self::CallDepth => "bf_err_call_depth",
            Self::StepLimit => "bf_err_internal",
            Self::Io => "bf_err_io",
        }
    }
}

/// An execution failure located at an instruction index (not a source offset).
#[derive(Debug)]
pub struct RuntimeError {
    pub kind: RuntimeErrorKind,
    pub pc: usize,
    pub message: String,
}

impl RuntimeError {
    pub(crate) fn new(kind: RuntimeErrorKind, pc: usize, message: impl Into<String>) -> Self {
        Self {
            kind,
            pc,
            message: message.into(),
        }
    }
}

impl fmt::Display for RuntimeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "runtime error [{}] at pc={}: {}",
            self.kind.status(),
            self.pc,
            self.message
        )
    }
}

impl std::error::Error for RuntimeError {}
