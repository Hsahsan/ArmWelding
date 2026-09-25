#include "brake_confirm.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

bool confirm_brake_released(const volatile sig_atomic_t *running)
{
    if (!*running) return false;
    // stdin can be a different terminal from /dev/tty under a PTY launcher.
    // Reopen the actual input terminal without changing stdin's shared flags.
    // A pipe/file is never accepted as an operator acknowledgement.
    char terminal_name[256]{};
    if (!isatty(STDIN_FILENO) || ttyname_r(STDIN_FILENO, terminal_name, sizeof(terminal_name)) != 0) {
        printf("Dibatalkan: konfirmasi rem memerlukan stdin dari terminal interaktif.\n");
        fflush(stdout);
        return false;
    }
    const int fd = open(terminal_name, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        printf("Dibatalkan: konfirmasi rem memerlukan terminal interaktif.\n");
        return false;
    }
    printf("Konfirmasi rem: menunggu jawaban operator di terminal input %s (akhiri dengan Enter).\n", terminal_name);
    fflush(stdout);
    const int printed = dprintf(fd,
        "\nRem memakai PSU 24 V eksternal; program tidak membaca status rem.\n"
        "Pastikan beban aman saat rem dilepas.\n"
        "Apakah rem sudah Anda lepas dengan memberi tegangan 24 V?\n"
        "Ketik REM LEPAS lalu Enter untuk melanjutkan; Enter/jawaban lain membatalkan: ");
    char answer[64]{};
    size_t used = 0;
    bool complete = false;
    bool invalid = false;
    while (printed >= 0 && *running && !complete) {
        // Attempt a nonblocking read before polling. A background PTY command
        // must request terminal input before a supervising launcher can resume it.
        char c;
        const ssize_t n = read(fd, &c, 1);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd terminal{fd, POLLIN, 0};
            const int ready = poll(&terminal, 1, 100);
            if (ready < 0 && errno != EINTR) break;
            if (terminal.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
            continue;
        }
        if (n != 1) break;
        if (c == '\n' || c == '\r') { complete = true; break; }
        if (c == '\003' || c == '\004') break; // Ctrl+C/EOF in a raw terminal.
        // Consume the whole line even if invalid; do not leave its tail for
        // the shell after cancelling this program.
        if (c == '\0' || used == sizeof(answer) - 1) { invalid = true; continue; }
        answer[used++] = c;
    }
    close(fd);
    const bool accepted = *running && complete && !invalid && strcmp(answer, "REM LEPAS") == 0;
    printf("\n%s\n", accepted
        ? "Operator mengonfirmasi REM LEPAS; melanjutkan mode yang dipilih."
        : "Dibatalkan: rem belum dikonfirmasi; koneksi EtherCAT tidak dibuka.");
    fflush(stdout);
    return accepted;
}
