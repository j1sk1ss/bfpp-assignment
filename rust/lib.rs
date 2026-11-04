//! Compile and execute BF/BF++ with caller-provided byte streams.
//!
//! ```
//! use bfpp::{Dialect, Program, VmConfig};
//! let program = Program::compile(b"+++++.", Dialect::Bfpp)?;
//! let mut output = Vec::new();
//! program.run(&mut &b""[..], &mut output, VmConfig::default())?;
//! assert_eq!(output, [5]);
//! # Ok::<(), Box<dyn std::error::Error>>(())
//! ```

mod compiler;
mod error;
mod tape;
mod vm;

pub use compiler::{Dialect, Instruction, Program};
pub use error::{CompileError, RuntimeError, RuntimeErrorKind};
pub use tape::TapeKind;
pub use vm::VmConfig;
