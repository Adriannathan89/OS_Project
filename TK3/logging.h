/* Bagian Adrian - log identik melalui stdio dan system call. */
#ifndef LOGGING_H
#define LOGGING_H

/* method: malloc/mmap; record sama dengan output terminal. */
int write_logs(const char *method, const char *record);

#endif
