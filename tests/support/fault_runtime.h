#ifndef FAULT_RUNTIME_H
# define FAULT_RUNTIME_H

# include <stddef.h>
# include <sys/types.h>

void	fault_runtime_reset(void);

void	fault_allocation_fail_at(size_t attempt);
size_t	fault_allocation_attempts(void);
int		fault_allocation_failed(void);
size_t	fault_live_allocations(void);
size_t	fault_invalid_frees(void);
size_t	fault_double_frees(void);

void	fault_read_fail_on(int fd, size_t call_index);
void	fault_read_limit(size_t limit);
size_t	fault_read_calls(void);
int		fault_read_failed(void);

void	*test_malloc(size_t size);
void	test_free(void *pointer);
ssize_t	test_read(int fd, void *buffer, size_t count);

#endif
