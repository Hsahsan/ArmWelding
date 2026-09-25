"""Run against CLI binaries linked with tests/no_bus_init.cpp; never use live binaries."""
from pathlib import Path
import fcntl
import os
import pty
import select
import signal
import subprocess
import termios
import tempfile
import time
import tty

root = Path(__file__).resolve().parents[1]
servo = root / "build/test_servo_no_bus"
diagnostic = root / "build/test_diagnostic_no_bus"
marker = b"TEST_BUS_INIT_REACHED_NO_SOCKET"
prompt = b"membatalkan: "
cases = 0
work = tempfile.TemporaryDirectory(prefix="lc-brake-test-")


def run_tty(binary, args, answer, expect_bus, expect_prompt=True, pipe_output=False,
            separate_stdin=False, live_output=False, raw_input=False):
    global cases
    master, slave = pty.openpty()
    input_master, input_slave = pty.openpty() if separate_stdin else (master, slave)
    if raw_input:
        tty.setraw(input_slave)

    def session():
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)

    env = dict(os.environ)
    if live_output:
        env["LC_TEST_PAUSE_INIT"] = "1"
    proc = subprocess.Popen([str(binary), "mock", *args], stdin=input_slave,
                            stdout=subprocess.PIPE if pipe_output else slave,
                            stderr=slave, preexec_fn=session, cwd=work.name,
                            pass_fds=(slave,), env=env)
    os.close(slave)
    if separate_stdin:
        os.close(input_slave)
    transcript = b""
    answered = False
    answered_at = None
    observed_live_output = False
    readers = {master, input_master}
    if pipe_output:
        readers.add(proc.stdout.fileno())
    deadline = time.monotonic() + 5
    try:
        while time.monotonic() < deadline:
            for fd in select.select(list(readers), [], [], 0.05)[0]:
                try:
                    chunk = os.read(fd, 65536)
                except OSError:
                    readers.discard(fd)
                    continue
                if not chunk:
                    readers.discard(fd)
                    continue
                transcript += chunk
            if prompt in transcript and not answered:
                if isinstance(answer, int):
                    os.kill(proc.pid, answer)
                else:
                    os.write(input_master, answer)
                answered = True
                answered_at = time.monotonic()
            if live_output and marker in transcript and proc.poll() is None:
                observed_live_output = True
            if live_output and answered_at and not observed_live_output:
                assert time.monotonic() - answered_at < 0.8, "Output buffered after confirmation: " + repr(transcript)
            if proc.poll() is not None and not readers:
                break
        code = proc.wait(timeout=1)
        if pipe_output:
            transcript += proc.stdout.read()
        assert (prompt in transcript) == expect_prompt, transcript
        assert (marker in transcript) == expect_bus, transcript
        if live_output:
            assert observed_live_output, transcript
        if expect_prompt and not expect_bus:
            assert code == 2, (code, transcript)
        cases += 1
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
        os.close(master)
        if separate_stdin:
            os.close(input_master)


args = ["--check-pdo-esi", "--confirm-brake"]
run_tty(servo, ["--test-rotate"], b"\n", False)
run_tty(servo, ["--test-rotate"], b"REM LEPAS\n", True, pipe_output=True)
run_tty(servo, ["--test-rotate", "--confirm-brake"], b"\n", False)
for answer in [b"\n", b"tidak\n", b"ya\n", b"REM LEPAS extra\n", b"x" * 80 + b"\n",
               b"\x04", signal.SIGINT, signal.SIGTERM]:
    run_tty(servo, args, answer, False)
run_tty(servo, args, b"REM LEPAS\n", True)
run_tty(servo, args, b"REM LEPAS\n", True, pipe_output=True)
run_tty(servo, args, b"REM LEPAS\n", True, pipe_output=True, separate_stdin=True)
run_tty(servo, args, b"REM LEPAS\n", True, pipe_output=True, live_output=True)
run_tty(diagnostic, ["--test-csp"], b"REM LEPAS\n", True, pipe_output=True, live_output=True)
run_tty(servo, args, b"REM LEPAS\r", True, pipe_output=True, raw_input=True)
run_tty(servo, args, b"\x03", False, raw_input=True)
run_tty(servo, args, b"\x04", False, raw_input=True)
for mode in [[], ["--confirm-brake"]]:
    run_tty(servo, mode, b"\n", False)
run_tty(servo, ["--check-pdo-esi"], b"", True, expect_prompt=False)
run_tty(servo, ["--invalid", "--confirm-brake"], b"", False, expect_prompt=False)
for mode in [[], ["--monitor"], ["--test-csp"], ["--test-csv"]]:
    run_tty(diagnostic, mode, b"\n", False)
run_tty(diagnostic, ["--test-csp"], b"REM LEPAS\n", True)
run_tty(diagnostic, ["--invalid"], b"", False, expect_prompt=False)

# A pipe containing the correct phrase must not bypass an interactive terminal.
for binary, mode in [(servo, args), (servo, []), (diagnostic, ["--test-csp"])]:
    result = subprocess.run([str(binary), "mock", *mode], input=b"REM LEPAS\n",
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            start_new_session=True, timeout=3, cwd=work.name)
    assert result.returncode == 2 and marker not in result.stdout, result.stdout
    cases += 1
print(f"PASS: {cases} confirmation/CLI scenarios; all EtherCAT initialization mocked, no sockets.")
work.cleanup()
