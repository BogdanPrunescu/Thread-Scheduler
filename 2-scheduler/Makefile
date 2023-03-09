CC = gcc
CFLAGS = -fPIC -Wall -g
LDFLAGS = -Wall -g

.PHONY: build
build: libscheduler.so

libscheduler.so: util/so_scheduler.o util/queue.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

so_scheduler.o: util/so_scheduler.c util/so_scheduler.h
	$(CC) $(CFLAGS) -o $@ -c $<
	
queue.o: util/queue.c util/queue.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so queue.o
