UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
  ESC := $(shell printf '\033')
  YELLOW := $(ESC)[1;33m
  RESET := $(ESC)[0m
else ifeq ($(UNAME_S),MINGW64_NT-10.0)
  ESC := $(shell printf '\033')
  YELLOW := $(ESC)[1;33m
  RESET := $(ESC)[0m
else
  YELLOW :=
  RESET :=
endif

export LC_ALL := C.UTF-8
export LANG := C.UTF-8

VERSION  = WD-10.21
TARGET   ?= watchdogs
CC       ?= gcc

CFLAGS   = -Os -pipe -s -flto -fdata-sections -ffunction-sections -fPIE -D_GNU_SOURCE -I/usr/include/openssl
LDFLAGS  = -Wl,-O1,--gc-sections -lm -lcurl -lreadline -lncurses -larchive -lssl -lcrypto

SRCS = chain.c hardware.c utils.c compiler.c archive.c curl.c package.c server.c crypto.c \
       tomlc99/toml.c cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean linux termux windows compress strip debug

all: $(TARGET)
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)\n"
	@$(MAKE) compress

$(TARGET): $(OBJS)
	@printf "$(YELLOW)==>$(RESET) Linking $(TARGET)\n"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@$(MAKE) strip

%.o: %.c
	@printf "$(YELLOW)==>$(RESET) Compiling $<\n"
	$(CC) $(CFLAGS) -c $< -o $@

strip:
	@if [ -f "$(TARGET)" ]; then \
		printf "$(YELLOW)==>$(RESET) Stripping binary...\n"; \
		strip --strip-all $(TARGET) || true; \
	else \
		printf "$(YELLOW)==>$(RESET) Nothing to strip\n"; \
	fi

compress:
	@if command -v upx >/dev/null 2>&1; then \
		printf "$(YELLOW)==>$(RESET) Compressing with UPX...\n"; \
		upx --best --lzma $(TARGET) || true; \
	else \
		printf "$(YELLOW)==>$(RESET) UPX not found, skipping compression.\n"; \
	fi

clean:
	rm -f $(OBJS) watchdogs watch_termux watchdogs.exe watchdogs.debug
	@printf "$(YELLOW)==>$(RESET) Clean done.\n"

linux:
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)\n"
	@$(MAKE) TARGET=watchdogs CC=gcc \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)"

termux:
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)\n"
	@$(MAKE) TARGET=watch_termux CC=clang \
	CFLAGS="$(CFLAGS) -D__ANDROID__ -fPIE" \
	LDFLAGS="$(LDFLAGS) -pie"

windows: $(OBJS)
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)\n"
	$(CC) $(CFLAGS) $(OBJS) -o watchdogs.exe \
		$(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)\n"

debug:
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG version with sanitizers\n"
	$(CC) -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined \
	-D_GNU_SOURCE -D_DBG_PRINT -I/usr/include/openssl \
	$(SRCS) -o watchdogs.debug \
	-lm -lcurl -ltinfo -lreadline -lncurses -larchive -lssl -lcrypto -pthread
	@printf "$(YELLOW)==>$(RESET) Debug build complete: ./watchdogs.debug\n"