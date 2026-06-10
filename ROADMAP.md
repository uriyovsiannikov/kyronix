# Kyronix — roadmap

Target: Linux-compatible x86-64 OS — statically and dynamically linked ELF binaries,
POSIX syscalls, real filesystem, networking.

`[x]` = done · `[~]` = partial/stub · `[ ]` = not started

---

## Реализовано (база)

- [x] GDT / IDT / TSS / PIC 8259 ремап
- [x] PMM (bitmap + free-stack), VMM (4-уровневый PT, NX, HHDM), heap
- [x] Demand paging — #PF handler выделяет страницу на лету (стек растёт автоматически)
- [x] PIT ~1000 Hz → g_ticks → вытесняющий планировщик (IRQ0)
- [x] SYSCALL/SYSRET + swapgs, TSS.rsp0, SSE
- [x] ELF64 загрузчик + argv/envp/auxv стек
- [x] fork / execve / wait4 / exit / exit_group
- [x] Кооперативный + вытесняющий планировщик
- [x] Сигналы: rt_sigaction, rt_sigreturn, kill, SIGCHLD
- [x] VFS ramfs + CPIO initrd, symlinks, chr-dev
- [x] /dev/tty, /dev/null, /dev/zero, /dev/stdin/stdout/stderr
- [x] pipe / pipe2, dup / dup2
- [x] 65+ syscall
- [x] Per-process cwd, chdir/getcwd
- [x] clock_gettime / gettimeofday (реальные ms с boot)
- [x] sbase утилиты + ksh shell

---

## P0 — Без этого ломается базовый shell (следующий спринт)

### VFS: запись и мутация файлов
- [ ] `O_CREAT` / `O_TRUNC` — создание и перезапись файлов в ramfs
- [ ] `write()` в существующий файл (сейчас только chr-dev пишут)
- [ ] `unlink()` / `rmdir()` — удаление
- [ ] `mkdir()` — syscall 83 (сейчас нет)
- [ ] `rename()` — syscall 82
- [ ] Shell-редиректы `>` `>>` без этого не работают

### nanosleep с реальным ожиданием
- [ ] Поле `wakeup_tick` в proc_t
- [ ] В sys_nanosleep: записать wakeup_tick = g_ticks + ms, перейти в PROC_WAITING
- [ ] В IRQ0: пробуждать процессы у которых wakeup_tick <= g_ticks
- [ ] Нужно для: `sleep 1`, таймаутов в shell, любых busy-wait замен

### SIGPIPE
- [ ] В pipe_write: если read_refs == 0 → доставить SIGPIPE писателю
- [ ] Сейчас pipe-команды зависают вместо завершения

---

## P1 — Нужно для запуска реального userspace (musl, busybox)

### /proc псевдофайловая система
- [ ] `/proc/self/exe` → symlink на исполняемый файл процесса
- [ ] `/proc/self/fd/N` → symlink на открытый fd
- [ ] `/proc/self/maps` → карта памяти (нужна gdb, sanitizers)
- [ ] `/proc/PID/` → базовые файлы (status, cmdline)
- [ ] `/proc/version`, `/proc/cpuinfo`, `/proc/meminfo`
- [ ] Реализация: chr-dev с динамической генерацией содержимого

### shebang (#!) в execve
- [ ] Парсить первые 2 байта: если `#!` — прочитать интерпретатор и перезапустить
- [ ] Нужен для: `#!/bin/sh`, `#!/usr/bin/env python`

### Настоящий getrandom
- [ ] RDRAND инструкция x86 (CPUID проверка)
- [ ] Fallback: XORshift на основе TSC + g_ticks если нет RDRAND
- [ ] musl libc инициализирует arc4random через getrandom

### futex (реальная блокировка)
- [ ] Список ожидания по адресу (futex_wait_queue)
- [ ] FUTEX_WAIT: если *uaddr == val → добавить в очередь, переключить контекст
- [ ] FUTEX_WAKE: разбудить N процессов из очереди
- [ ] Нужен для: любой многопоточной программы, musl mutex

### mmap file-backed (MAP_SHARED / MAP_PRIVATE)
- [ ] `mmap(fd, offset, len)` — отобразить содержимое файла в память
- [ ] Нужен для: динамический линкёр, `dlopen`, большинство больших программ

---

## P2 — Для запуска динамически слинкованных программ

### Динамический линкёр
- [ ] Поддержка `PT_INTERP` в ELF — загружать ld.so
- [ ] `AT_BASE` в auxv — база линкёра
- [ ] Нужна поддержка mmap file-backed (P1)
- [ ] Собрать musl libc как shared library

### /dev/random, /dev/urandom
- [ ] chr-dev использующий getrandom (P1)

### select / poll реальная блокировка
- [ ] Сейчас всегда возвращает "готово" — ломает event loop программы
- [ ] Нужно: список fd-ожиданий, разбудить на событии read/write-ready

### Потоки (clone/pthread)
- [ ] `clone()` syscall с флагами CLONE_VM / CLONE_FS / CLONE_FILES
- [ ] TLS: каждый поток имеет свой FS base
- [ ] `set_tid_address`, `exit_thread` (не exit_group)
- [ ] Нужен для: любой программы использующей pthreads

---

## P3 — Постоянное хранилище и реальная ФС

### Драйвер virtio-blk
- [ ] PCI enumeration (поиск device 1af4:1001)
- [ ] Virtio ring setup (virtqueue)
- [ ] Чтение/запись секторов

### Файловая система ext2
- [ ] Парсинг суперблока, inodes, блоков
- [ ] Монтирование как root или /mnt
- [ ] Создание файлов, директорий, запись
- [ ] Возможность устанавливать программы постоянно

### Обновлённый VFS: mount table
- [ ] vfs_mount(path, fs_type, device)
- [ ] Поддержка нескольких ФС одновременно

---

## P4 — Сеть

### virtio-net драйвер
- [ ] Virtio NIC (device 1af4:1000)
- [ ] Отправка/приём ethernet фреймов

### Сетевой стек
- [ ] ARP, IPv4, ICMP (ping)
- [ ] UDP
- [ ] TCP (state machine: SYN/ACK/FIN)

### BSD сокеты
- [ ] `socket()` / `bind()` / `connect()` / `send()` / `recv()`
- [ ] AF_INET + SOCK_STREAM / SOCK_DGRAM
- [ ] AF_UNIX (unix domain sockets — нужны для dbus, systemd-style IPC)

---

## P5 — Полноценная совместимость

### APIC вместо PIC 8259
- [ ] Local APIC: per-CPU timer (замена PIT)
- [ ] I/O APIC: маршрутизация IRQ
- [ ] Нужен для SMP (> 1 ядра)

### SMP (мультипроцессорность)
- [ ] AP startup (SIPI sequence)
- [ ] Per-CPU структуры (GDT/IDT/TSS/планировщик)
- [ ] Spinlock / RCU примитивы

### Полная модель прав
- [ ] uid/gid реально проверяются в VFS
- [ ] `setuid` / `setgid` / capabilities
- [ ] chroot

### TTY/PTY
- [ ] `/dev/ptmx` — мастер сторона псевдотерминала
- [ ] `/dev/pts/N` — slave сторона
- [ ] Нужен для: SSH, tmux, screen

---

## Порядок работы (рекомендуемый)

```
P0: VFS write → nanosleep → SIGPIPE
P1: /proc → shebang → getrandom RDRAND → futex → mmap file-backed
P2: динамический линкёр → clone/threads → poll блокировка
P3: virtio-blk → ext2 → mount table
P4: virtio-net → TCP/IP → сокеты
P5: APIC → SMP → PTY → права
```
