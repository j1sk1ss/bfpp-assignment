use std::{env, ffi::OsString, fs, io, process::ExitCode};

use bfpp::{Dialect, Program, TapeKind, VmConfig};

fn usage(name: &OsString) -> ExitCode {
    eprintln!("Usage: {} [--dialect=bf|bfpp] [--tape=fixed|grow] <file>\n  fixed: 30000 cells\n  grow : starts at 1024 cells, grows up to 1 MiB", name.to_string_lossy());
    ExitCode::from(2)
}

fn main() -> ExitCode {
    let mut args = env::args_os();
    let name = args.next().unwrap_or_else(|| "bfpp".into());
    let mut path = None;
    let mut dialect = String::from("bfpp");
    let mut tape = String::from("grow");
    for arg in args {
        let text = arg.to_string_lossy();
        if let Some(value) = text.strip_prefix("--dialect=") {
            dialect = value.into();
        } else if let Some(value) = text.strip_prefix("--tape=") {
            tape = value.into();
        } else if text.starts_with('-') || path.is_some() {
            return usage(&name);
        } else {
            path = Some(arg);
        }
    }
    let Some(path) = path else {
        return usage(&name);
    };
    match execute(path, &dialect, &tape) {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("{message}");
            ExitCode::FAILURE
        }
    }
}

fn execute(path: OsString, dialect: &str, tape: &str) -> Result<(), String> {
    let source =
        fs::read(&path).map_err(|e| format!("cannot read '{}': {e}", path.to_string_lossy()))?;
    let dialect = match dialect {
        "bf" => Dialect::Bf,
        "bfpp" => Dialect::Bfpp,
        _ => return Err(format!("unknown dialect: {dialect}")),
    };
    let program = Program::compile(&source, dialect).map_err(|e| e.to_string())?;
    let tape = match tape {
        "fixed" => TapeKind::Fixed,
        "grow" => TapeKind::Grow,
        _ => return Err(format!("unknown tape: {tape}")),
    };
    program
        .run(
            &mut io::stdin().lock(),
            &mut io::stdout().lock(),
            VmConfig {
                tape,
                ..VmConfig::default()
            },
        )
        .map_err(|e| e.to_string())?;
    Ok(())
}
