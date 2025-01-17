#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

# if BUFFER_SIZE <= 0
#  error "BUFFER_SIZE must be greater than zero"
# endif

char	*get_next_line(int fd);

#endif
