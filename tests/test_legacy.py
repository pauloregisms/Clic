"""Compare the eight original examples against the version before networking.

Uses real PTYs, compiles both versions with warnings as errors, and never connects
outside this machine. Requires git history for the baseline (overridable by env).
"""
import errno
import fcntl
import os
from pathlib import Path
import pty
import select
import shlex
import struct
import subprocess
import tempfile
import termios
import time

ROOT = Path(__file__).resolve().parents[1]
BASELINE = os.environ.get("CLIC_BASELINE_REF", "41c095e")


def terminal_run(executable, kind):
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    settings = termios.tcgetattr(slave)
    settings[3] &= ~termios.ECHO
    termios.tcsetattr(slave, termios.TCSANOW, settings)
    process = subprocess.Popen(
        [str(executable)], stdin=slave, stdout=slave, stderr=slave,
        env={**os.environ, "LC_ALL": "C.UTF-8", "TERM": "xterm-256color"},
    )
    os.close(slave)
    actions = {
        3: [(b"Pressione as setas", b"\x1b[B"), (b"Direcao: baixo", b"q")],
        4: [(b"Sair", b"\x1b[B\n")],
        6: [(b"Sair", b"\n")],
        7: [(b"Pressione q", b"q")],
    }.get(kind, [])
    output = b""
    stage = 0
    deadline = time.monotonic() + 5
    try:
        while time.monotonic() < deadline:
            if not select.select([master], [], [], 0.1)[0]:
                continue
            try:
                chunk = os.read(master, 65536)
            except OSError as error:
                if error.errno == errno.EIO:
                    break
                raise
            if not chunk:
                break
            output += chunk
            if stage < len(actions) and actions[stage][0] in output:
                # The library restores echo between key reads; suppress input
                # echo so scheduling cannot alter the captured terminal output.
                settings = termios.tcgetattr(master)
                settings[3] &= ~termios.ECHO
                termios.tcsetattr(master, termios.TCSANOW, settings)
                os.write(master, actions[stage][1])
                stage += 1
        else:
            raise AssertionError(f"Timed out: {executable.name}")
        assert process.wait(timeout=1) == 0, (executable.name, output)
        assert stage == len(actions), (executable.name, "missing keyboard interaction")
        return output
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        os.close(master)


def main():
    with tempfile.TemporaryDirectory(prefix="clic-legacy-") as temporary:
        base = Path(temporary)
        for name in ("Clic.c", "Clic.h"):
            (base / name).write_bytes(subprocess.check_output(
                ["git", "show", f"{BASELINE}:{name}"], cwd=ROOT,
            ))
        count = 0
        for source in sorted((ROOT / "exemplos").glob("*.c")):
            kind = int(source.name[:2])
            outputs = []
            for label, library in (("base", base), ("new", ROOT)):
                executable = base / f"{label}_{source.stem}"
                subprocess.run([
                    *shlex.split(os.environ.get("CC", "cc")),
                    "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                    f"-I{library}", str(source), str(library / "Clic.c"),
                    "-o", str(executable),
                ], check=True)
                outputs.append(terminal_run(executable, kind))
            if kind == 7:
                # Frame counts depend on scheduling, not API behaviour.
                assert all(b"Animacao encerrada." in output for output in outputs)
            else:
                assert outputs[0] == outputs[1], f"Terminal output changed: {source.name}"
            print(f"PASS legacy {source.name}")
            count += 1
        assert count == 8, f"Expected eight original examples, found {count}"
        print(f"{count} legacy examples, 0 failures")


if __name__ == "__main__":
    main()
