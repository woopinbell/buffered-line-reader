#include "test.h"

#include <stdio.h>

static int	g_checks;
static int	g_failures;

void	test_check(int condition, const char *expression, const char *file,
		int line)
{
	g_checks++;
	if (!condition)
	{
		g_failures++;
		fprintf(stderr, "%s:%d: check failed: %s\n", file, line,
			expression);
	}
}

int	test_finish(void)
{
	if (g_failures != 0)
	{
		fprintf(stderr, "%d of %d checks failed\n", g_failures, g_checks);
		return (1);
	}
	printf("%d checks passed\n", g_checks);
	return (0);
}

int	main(void)
{
	test_reader();
	test_boundaries();
	test_context();
	return (test_finish());
}
