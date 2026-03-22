# Changelog

All notable changes to this project will be documented in this file.

The format is inspired by Keep a Changelog, and versions follow Semantic Versioning.

## [Unreleased]

### Planned

- Stabilize API naming and complete module coverage for SPI, I2C, ADC, and DMA.
- Continue MISRA-focused cleanup and static analysis tuning for embedded targets.

## [0.1.0] - 2026-03-22

### Added

- Initial project scaffold with top-level build and documentation files.
- Header-only HAL modules for RCC and SysTick.
- Additional HAL modules: Core, EXTI, GPIO, PFIC, PWR, and USART.
- Module documentation files under `docs/` for Core, PWR, RCC, EXTI, GPIO/AFIO, and USART.

### Changed

- Expanded `README.md` with project overview, status, build guidance, and design rationale.
- Updated `docs/01_core.md` and `docs/highcode.md` with refined implementation notes.
- Updated `include/hal/rcc.h` with additional support details.

### Renamed

- `docs/EXTIn.md` to `docs/06_exti.md`.
- `docs/GPIO_and_AFIO_doc.md` to `docs/07_gpio_afio.md`.
- `docs/USART_doc.md` to `docs/12_usart.md`.

### Removed

- Removed legacy `include/hal/lib.h`.
- Removed legacy `docs/doc.md`.
