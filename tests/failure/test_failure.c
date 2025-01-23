#include "fault_runtime.h"
#include "get_next_line.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int	g_checks;
static int	g_failures;

static void	check(int condition, const char *expression, int line)
{
	g_checks++;
	if (!condition)
	{
		g_failures++;
		fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, line,
			expression);
	}
}

#define CHECK(condition) check((condition), #condition, __LINE__)

static int	write_all(int fd, const char *bytes, size_t length)
{
	ssize_t	written;

	while (length > 0)
	{
		written = write(fd, bytes, length);
		if (written <= 0)
			return (0);
		bytes += written;
		length -= (size_t)written;
	}
	return (1);
}

static int	reader_for(const char *bytes)
{
	int	fds[2];

	if (pipe(fds) != 0)
		return (-1);
	if (!write_all(fds[1], bytes, strlen(bytes)))
	{
		close(fds[0]);
		close(fds[1]);
		return (-1);
	}
	close(fds[1]);
	return (fds[0]);
}

static void	check_clean_runtime(void)
{
	CHECK(fault_live_allocations() == 0);
	CHECK(fault_invalid_frees() == 0);
	CHECK(fault_double_frees() == 0);
}

static size_t	consume_all(size_t fail_at, int *failure_observed)
{
	char	*line;
	int		fd;
	size_t	attempts;

	fault_runtime_reset();
	fault_allocation_fail_at(fail_at);
	fd = reader_for("alpha\nbeta\nlast");
	CHECK(fd >= 0);
	if (fd < 0)
		return (0);
	line = get_next_line(fd);
	while (line != NULL)
	{
		test_free(line);
		line = get_next_line(fd);
	}
	attempts = fault_allocation_attempts();
	*failure_observed = fault_allocation_failed();
	close(fd);
	check_clean_runtime();
	return (attempts);
}

static void	test_allocation_failures(void)
{
	int		failed;
	size_t	attempts;
	size_t	baseline;
	size_t	index;

	baseline = consume_all(0, &failed);
	CHECK(!failed);
	CHECK(baseline > 0);
	index = 1;
	while (index <= baseline)
	{
		attempts = consume_all(index, &failed);
		CHECK(failed);
		CHECK(attempts == index);
		index++;
	}
	consume_all(baseline + 1, &failed);
	CHECK(!failed);
}

static void	test_short_reads(void)
{
	char	*line;
	int		fd;

	fault_runtime_reset();
	fd = reader_for("short reads still work\nlast");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	fault_read_fail_on(fd, 0);
	fault_read_limit(3);
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "short reads still work\n") == 0);
	test_free(line);
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "last") == 0);
	test_free(line);
	CHECK(get_next_line(fd) == NULL);
	CHECK(fault_read_calls() > 2);
	CHECK(!fault_read_failed());
	close(fd);
	check_clean_runtime();
}

static void	test_first_read_error(void)
{
	int	fd;

	fault_runtime_reset();
	fd = reader_for("unread input");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	fault_read_fail_on(fd, 1);
	CHECK(get_next_line(fd) == NULL);
	CHECK(fault_read_calls() == 1);
	CHECK(fault_read_failed());
	close(fd);
	check_clean_runtime();
}

static void	test_middle_read_error(void)
{
	int	fd;

	fault_runtime_reset();
	fd = reader_for("partial bytes must be discarded");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	fault_read_limit(4);
	fault_read_fail_on(fd, 3);
	CHECK(get_next_line(fd) == NULL);
	CHECK(fault_read_calls() == 3);
	CHECK(fault_read_failed());
	close(fd);
	check_clean_runtime();
}

static void	test_one_fd_failure_preserves_another(void)
{
	char	*line;
	int		left;
	int		right;

	fault_runtime_reset();
	left = reader_for("left one\nleft two");
	right = reader_for("right stream fails");
	CHECK(left >= 0 && right >= 0);
	if (left < 0 || right < 0)
		return ;
	line = get_next_line(left);
	CHECK(line != NULL && strcmp(line, "left one\n") == 0);
	test_free(line);
	fault_read_fail_on(right, 1);
	CHECK(get_next_line(right) == NULL);
	CHECK(fault_read_failed());
	line = get_next_line(left);
	CHECK(line != NULL && strcmp(line, "left two") == 0);
	test_free(line);
	CHECK(get_next_line(left) == NULL);
	close(left);
	close(right);
	check_clean_runtime();
}

static void	test_free_guards(void)
{
	char	local;
	void	*allocation;

	fault_runtime_reset();
	allocation = test_malloc(1);
	CHECK(allocation != NULL);
	test_free(allocation);
	test_free(allocation);
	test_free(&local);
	CHECK(fault_live_allocations() == 0);
	CHECK(fault_double_frees() == 1);
	CHECK(fault_invalid_frees() == 1);
	fault_runtime_reset();
	check_clean_runtime();
}

static void	test_context_retries_line_allocation(void)
{
	char			*line;
	int				fd;
	t_blr_reader	*reader;

	fault_runtime_reset();
	fd = reader_for("\n");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	reader = blr_reader_create(fd);
	CHECK(reader != NULL);
	if (reader == NULL)
	{
		close(fd);
		return ;
	}
	fault_allocation_fail_at(3);
	line = (char *)reader;
	CHECK(blr_reader_next(reader, &line) == BLR_ERROR);
	CHECK(line == NULL);
	CHECK(fault_allocation_failed());
	fault_allocation_fail_at(0);
	CHECK(blr_reader_next(reader, &line) == BLR_LINE);
	CHECK(line != NULL && strcmp(line, "\n") == 0);
	test_free(line);
	CHECK(blr_reader_next(reader, &line) == BLR_EOF);
	CHECK(line == NULL);
	blr_reader_destroy(reader);
	close(fd);
	check_clean_runtime();
}

static size_t	consume_context_tail(size_t fail_at, int *failed)
{
	char			*line;
	int				fd;
	int				seen;
	size_t			attempts;
	t_blr_reader	*reader;
	t_blr_result	result;

	fault_runtime_reset();
	fault_allocation_fail_at(fail_at);
	fd = reader_for("tail");
	CHECK(fd >= 0);
	if (fd < 0)
		return (0);
	reader = blr_reader_create(fd);
	seen = 0;
	result = BLR_ERROR;
	while (reader != NULL && result != BLR_EOF)
	{
		result = blr_reader_next(reader, &line);
		if (result == BLR_ERROR && fault_allocation_failed())
			fault_allocation_fail_at(0);
		else if (result == BLR_LINE)
		{
			CHECK(!seen);
			CHECK(line != NULL && strcmp(line, "tail") == 0);
			seen = 1;
			test_free(line);
		}
	}
	CHECK(reader != NULL);
	CHECK(seen);
	attempts = fault_allocation_attempts();
	*failed = fault_allocation_failed();
	blr_reader_destroy(reader);
	close(fd);
	check_clean_runtime();
	return (attempts);
}

static void	test_context_retries_final_line_allocation(void)
{
	int		failed;
	size_t	baseline;
	size_t	index;

	baseline = consume_context_tail(0, &failed);
	CHECK(!failed);
	CHECK(baseline > 0);
	index = 2;
	while (index <= baseline)
	{
		consume_context_tail(index, &failed);
		CHECK(failed);
		index++;
	}
}

int	main(void)
{
	test_allocation_failures();
	test_short_reads();
	test_first_read_error();
	test_middle_read_error();
	test_one_fd_failure_preserves_another();
	test_context_retries_line_allocation();
	test_context_retries_final_line_allocation();
	test_free_guards();
	if (g_failures != 0)
	{
		fprintf(stderr, "%d of %d checks failed\n", g_failures, g_checks);
		return (1);
	}
	printf("%d failure checks passed\n", g_checks);
	return (0);
}
