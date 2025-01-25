# Buffered line reader

![Language](https://img.shields.io/badge/language-C99-blue?logo=c&logoColor=white)
![Platform](https://img.shields.io/badge/platform-POSIX-lightgrey)

`buffered_line_reader`는 42 `get_next_line` 과제를 변형한 POSIX 파일 디스크립터에서 한 줄씩 읽는 C99 정적 라이브러리입니다. 

## 주요 API

| API | 용도 |
| --- | --- |
| `get_next_line` | 다음 줄을 `char *`로 반환하는 호환 API |
| `blr_reader_create` | 파일 디스크립터를 사용하는 읽기 컨텍스트 생성 |
| `blr_reader_next` | 상태값과 함께 다음 줄 읽기 |
| `blr_reader_reset` | 누적 버퍼와 읽기 상태 초기화 |
| `blr_reader_destroy` | 컨텍스트와 내부 메모리 해제 |

`blr_reader_next`는 다음 상태를 반환합니다.

- `BLR_LINE`: 줄을 읽었으며 `*line`을 호출자가 해제합니다.
- `BLR_EOF`: 더 읽을 데이터가 없습니다.
- `BLR_AGAIN`: 비차단 입력이 아직 준비되지 않았으므로 다시 호출해야 합니다.
- `BLR_ERROR`: 읽기 또는 메모리 오류가 발생했습니다.

## 빌드

```sh
make
```

정적 라이브러리는 `build/libbuffered_line_reader.a`로 생성됩니다. 기본 `BUFFER_SIZE`는 42이며 빌드할 때 변경할 수 있습니다.

```sh
make re BUFFER_SIZE=1024
```

## 사용 예시

```c
#include "get_next_line.h"
#include <stdlib.h>

char *line;

line = get_next_line(fd);
if (line != NULL) {
	  free(line);
}
```

프로그램과 링크하려면 헤더 경로와 정적 라이브러리를 지정합니다.

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic \
	  -Iinclude example.c build/libbuffered_line_reader.a -o example
```

## 테스트

```sh
make test
make test-asan
```

`make test`는 기본 기능과 경계 조건을 검사하고, `make test-asan`은 AddressSanitizer와 UndefinedBehaviorSanitizer를 사용해 같은 테스트를 실행합니다. `BUFFER_SIZE`를 지정해 버퍼 크기를 바꿔 테스트할 수 있습니다.

```sh
make test BUFFER_SIZE=1024
```

## 사용 시 주의사항

- 컨텍스트는 파일 디스크립터를 소유하지 않으므로 닫지 않습니다.
- 같은 open file description을 공유하는 `dup` 계열 디스크립터에는 컨텍스트 하나만 사용합니다.
- 외부에서 `lseek`로 파일 위치를 변경했다면 `blr_reader_reset`을 호출합니다.
- 하나의 컨텍스트를 동시에 호출할 수 없습니다.
- 반환된 문자열은 NUL 종료 문자열이며 embedded NUL을 포함한 바이너리 데이터의 길이는 표현하지 않습니다.
- `get_next_line`은 내부 전역 상태를 사용하므로 같은 파일 디스크립터에 대한 병렬 호출을 지원하지 않습니다.

## 정리

```sh
make clean  # 빌드 및 테스트 산출물 삭제
make fclean # clean 후 정적 라이브러리 삭제
make re     # fclean 후 재빌드
```
