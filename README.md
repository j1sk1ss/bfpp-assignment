# BFPP на Rust

Интерпретатор BF и BF++ с совместимым CLI и библиотекой без внешних
зависимостей. Требуется stable Rust. Спецификация языка: [language_specs.md](language_specs.md).

## Сборка и запуск

```sh
cargo +stable build --locked --release --bin bfpp
./target/release/bfpp examples/lines.bfpp
./target/release/bfpp --dialect=bf --tape=fixed examples/hello.bf
printf ABC | ./target/release/bfpp examples/lines.bfpp
```

CLI: `bfpp [--dialect=bf|bfpp] [--tape=fixed|grow] <file>`.
По умолчанию используются `bfpp` и `grow`. Опции можно указывать до или после
пути; при повторении опции используется последнее значение.

- `fixed`: 30 000 ячеек.
- `grow`: изначально 1024 ячейки, удвоение до 1 048 576 ячеек.
- Ячейки — байты с арифметикой по модулю 256; лента растёт только вправо.
- stdin/stdout передают байты, в том числе NUL и значения 128–255. EOF записывает 0.
- Код завершения: `0` — успех, `1` — ошибка чтения, компиляции или выполнения,
  `2` — неверная форма команды. Диагностика идёт в stderr.

## Библиотека и архитектура

```rust
use bfpp::{Dialect, Program, VmConfig};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let program = Program::compile(b",. *++++++++++.", Dialect::Bfpp)?;
    let mut output = Vec::new();
    program.run(&mut &b"A"[..], &mut output, VmConfig::default())?;
    assert_eq!(output, b"A\n");
    Ok(())
}
```

`Program` содержит типизированные инструкции и таблицу меток. Компилятор
разрешает переходы и циклы до выполнения. Метод `label` даёт доступ к меткам,
`instructions` — к неизменяемому байткоду. Эти данные хранятся отдельно от
стека вызовов и могут использоваться при разработке отладчика.

VM использует явный `Vec` кадров без рекурсии в Rust. Каждый вызов получает
свою нулевую ленту, указатель, исходящие аргументы и последнее возвращённое
значение. `#0`–`#9` читают аргументы вызывающего кадра; в основной программе
они ничего не меняют. `!` возвращает текущую ячейку или завершает основную
программу. Достижение конца кода завершает всю программу без неявного возврата.
Первое определение повторяющейся метки сохраняется. Соседние `+`/`-` и `>`/`<`
сворачиваются в сумму, включая проверку границ ленты по итоговому сдвигу.

`Program::run` принимает любые `Read`/`Write` и при каждом запуске создаёт
новое состояние. Ошибки ввода, как в исходной C-версии, записывают 0; ошибки
вывода и сброса буфера возвращают `RuntimeError`. `VmConfig` позволяет выбрать
ленту, ограничить число инструкций и глубину вызовов. По умолчанию глубина
ограничена 128 кадрами, включая основной, число инструкций не ограничено.
Значение глубины 0 выбирает совместимый с C API предел 64.
`CompileError` содержит смещение в байтах исходника, `RuntimeError` — индекс
инструкции и категорию ошибки. Документация API: `cargo +stable doc --no-deps`.

Исходная реализация на C сохранена в `src/` и `include/` для сравнения;
Rust-реализация находится в `rust/`. Существующие тесты и фикстуры сохранены.
Cargo и текущий GitHub Actions workflow собирают Rust; `make` по-прежнему
собирает исходную C-версию.

## Проверки

```sh
cargo +stable fmt --all --check
cargo +stable check --locked --workspace --all-targets --all-features
cargo +stable clippy --locked --workspace --all-targets --all-features -- -D warnings
cargo +stable test --locked --workspace --all-features
cargo +stable build --locked --release --bin bfpp
python3 -B -m unittest discover -s tests -p test_bfpp_runner.py
python3 -B tests/run_bfpp_tests.py -- ./target/release/bfpp
python3 -B tests/test_cli.py ./target/release/bfpp
```

Rust-тесты дополнительно проверяют исходные фикстуры компилятора, диагностику,
границы обеих лент, изоляцию вызовов, лимиты выполнения и ошибки вывода.
