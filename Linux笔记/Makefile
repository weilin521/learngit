#三要素:目标、依赖、命令
#目标：依赖（条件）
#    命令(前面有个tab键)
#目标(顶格):依赖条件
#阶段一:自顶向下建立关系
#app:add.c sub.c dive.c mul.c main.c
#	gcc add.c sub.c dive.c mul.c main.c -o app
#gcc -c add.c(-c是只编译，不链接)

#阶段二:自下向上去执行对应的命令

#app:add.o sub.o dive.o mul.o main.o
#	gcc add.o sub.o dive.o mul.o main.o -o app

#add.o:add.c
#	gcc -c add.c
#sub.o:sub.c
#	gcc -c sub.c
#dive.o:dive.c
#	gcc -c dive.c
#mul.o:mul.c
#	gcc -c mul.c
#main.o:main.c
#	gcc -c main.c


#阶段三
#@表示目标，$^表示有依赖，$<表示依赖中的第一个
#obj=add.o sub.o mul.o dive.o main.o
#wildcard用来查询符合的文件
# 模式查询替换,从src中把.c文件替换成为.o文件
#src = $(wildcard *.c)
#obj = $(patsubst %.c,%.o,$(src))
#app:$(obj)
#	gcc $(obj) -o app

#target = app

#$(target):$(obj)
#	gcc $^ -o $@

#%.o:%.c
#	gcc -c $< -o $@

#阶段四

CPPFLAGS= -Iinclude
CFLAGS= -g -Wall
LDFLAGS= 
CC= gcc

src = $(wildcard *.c)
obj = $(patsubst %.c,%.o,$(src))
 
target = app
 
$(target):$(obj)
	$(CC) $^ $(LDFLAGS) -o $@
 
%.o:%.c
	$(CC) -c $< $(CFLAGS) $(CPPFLAGS) -o $@


.PHONY:clean
clean:
	-rm -f *.o
	-rm -f app

text:
	@echo "hello"
	echo $(src)
	@echo $(obj)
