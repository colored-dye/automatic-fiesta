all:
	make -C client
	make -C server

.PHONY: clean
clean:
	make -C ./client clean
	make -C server clean
