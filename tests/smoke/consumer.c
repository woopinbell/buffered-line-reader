#include "get_next_line.h"

#include <stdlib.h>

int	main(void)
{
	char	*line;

	line = get_next_line(-1);
	if (line != NULL)
	{
		free(line);
		return (1);
	}
	return (0);
}
