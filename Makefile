VERSION  = WD-10.18

TARGET   ?= watchdogs
CC       ?= gcc

CFLAGS   = -fPIE -D_GNU_SOURCE -g -O2 -pipe -s -flto -I/usr/include/openssl
LDFLAGS  = -lm -lcurl -ltinfo -lreadline -lncurses -larchive -lssl -lcrypto -Wl,-O1

SRCS = watchdogs.c utils.c archive.c curl.c package.c server.c crypto.c \
       tomlc99/toml.c cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean linux termux mingw64 debug

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "==> Linking $(TARGET)"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	@mkdir -p $(dir $@)
	@echo "==> Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) watchdogs watchdogs_termux watchdogs.exe watchdogs.debug
	@echo "Clean done."

linux:
	@$(MAKE) TARGET=watchdogs CC=gcc \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)"

termux:
	@$(MAKE) TARGET=watchdogs_termux CC=clang \
	CFLAGS="$(CFLAGS) -D__ANDROID__ -fPIE" \
	LDFLAGS="$(LDFLAGS) -pie"

mingw64:
	@$(MAKE) TARGET=watchdogs.exe CC=x86_64-w64-mingw32-gcc \
	CFLAGS="$(CFLAGS) -DWIN32 -D_WIN32 -fno-pie" \
	LDFLAGS="-lws2_32 -lwinmm -static"

debug:
	@echo "==> Building DEBUG version with sanitizers"
	$(CC) -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-optimize-sibling-calls \
	-D_GNU_SOURCE -I/usr/include/openssl \
	$(SRCS) -o watchdogs.debug \
	-lm -lcurl -ltinfo -lreadline -lncurses -larchive -lssl -lcrypto -pthread
	@echo "==> Debug build complete: ./watchdogs.debug"

