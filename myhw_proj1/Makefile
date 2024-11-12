# 컴파일러와 플래그 설정
CC = gcc
CFLAGS = -Wall -g

# 실행 파일 이름
TARGET = sec_program

# 소스 및 헤더 파일
SRCS = main.c queue.c
HEADERS = msg.h

# 객체 파일 생성
OBJS = $(SRCS:.c=.o)

# 기본 빌드 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 개별 소스 파일 컴파일
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 깨끗한 빌드 (중간 파일 삭제)
clean:
	rm -f $(OBJS) $(TARGET)

# 실행 파일 실행 (명령줄 인수 받기)
run: $(TARGET)
	./$(TARGET) $(ARGS)

