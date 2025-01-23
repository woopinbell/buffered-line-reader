#ifndef GET_NEXT_LINE_H
# define GET_NEXT_LINE_H

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 42
# endif

# if BUFFER_SIZE <= 0
#  error "BUFFER_SIZE must be greater than zero"
# endif

typedef struct s_blr_reader	t_blr_reader;

typedef enum e_blr_result
{
	BLR_ERROR = -1,
	BLR_EOF = 0,
	BLR_LINE = 1
}	t_blr_result;

/*
 * 컨텍스트는 fd를 빌릴 뿐 닫지 않습니다. 같은 open file description을
 * 공유하는 dup 계열 fd에는 컨텍스트를 하나만 두어야 하며, 외부에서
 * 오프셋을 바꾼 뒤에는 blr_reader_reset을 호출해야 합니다. 서로 독립된
 * 컨텍스트는 다른 스레드에서 사용할 수 있지만 한 컨텍스트를 동시에
 * 호출하는 동작은 지원하지 않습니다. reset은 누적 상태만 버리고 fd나
 * 현재 오프셋은 바꾸지 않습니다. 컨텍스트가 살아 있는 동안 fd를 닫아
 * 같은 번호로 다시 열었다면 기존 컨텍스트를 재사용하지 않아야 합니다.
 *
 * BLR_LINE이면 *line은 호출자가 free해야 합니다. 나머지 결과에서는
 * *line을 NULL로 설정합니다. BLR_ERROR 뒤에도 reset하거나 destroy할 수
 * 있습니다.
 */
t_blr_reader	*blr_reader_create(int fd);
t_blr_result	blr_reader_next(t_blr_reader *reader, char **line);
void			blr_reader_reset(t_blr_reader *reader);
void			blr_reader_destroy(t_blr_reader *reader);

char	*get_next_line(int fd);

#endif
