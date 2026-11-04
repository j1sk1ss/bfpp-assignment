/// Tape allocation policy. Both policies reject positions left of cell zero.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TapeKind {
    /// Exactly 30,000 zero-initialized byte cells.
    Fixed,
    /// Starts with 1,024 cells and doubles up to 1 MiB.
    #[default]
    Grow,
}

pub(crate) struct Tape {
    cells: Vec<u8>,
    position: usize,
    limit: usize,
}

impl Tape {
    pub(crate) fn new(kind: TapeKind) -> Self {
        let (initial, limit) = match kind {
            TapeKind::Fixed => (30_000, 30_000),
            TapeKind::Grow => (1024, 1024 * 1024),
        };
        Self {
            cells: vec![0; initial],
            position: 0,
            limit,
        }
    }

    pub(crate) fn cell(&mut self) -> &mut u8 {
        &mut self.cells[self.position]
    }

    pub(crate) fn move_by(&mut self, delta: i32) -> Result<(), ()> {
        let target = (self.position as i64) + i64::from(delta);
        if target < 0 || target >= self.limit as i64 {
            return Err(());
        }
        let target = target as usize;
        if target >= self.cells.len() {
            let mut capacity = self.cells.len();
            while capacity <= target {
                capacity = (capacity * 2).min(self.limit);
            }
            self.cells.resize(capacity, 0);
        }
        self.position = target;
        Ok(())
    }
}
