#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

# if BUFFER_SIZE <= 0
#  error "BUFFER_SIZE must be greater than zero"
# endif

typedef struct s_blr_reader	t_blr_reader;

t_blr_reader	*blr_reader_create(int fd);
void			blr_reader_reset(t_blr_reader *reader);
void			blr_reader_destroy(t_blr_reader *reader);

char	*get_next_line(int fd);

#endif
