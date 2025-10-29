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

export LANG := C.UTF-8
export LC_ALL := C.UTF-8

VERSION  	 = WD-10.21
FULL_VERSION = WD-25.10.21.0
TARGET   	 ?= watchdogs
CC       	 ?= clang
STRIP        ?= llvm-strip

CFLAGS   = -Os -pipe -s -fdata-sections -ffunction-sections -fPIE
LDFLAGS  = -Wl,-O1,--gc-sections -lm -lcurl -lreadline -lncurses -larchive -lssl -lcrypto

SRCS = extra.c chain.c utils.c depends.c hardware.c compiler.c archive.c curl.c package.c server.c crypto.c \
       include/tomlc/toml.c include/cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: init all clean linux termux windows compress strip debug termux-debug windows-debug

init:
	@echo "$(YELLOW)==>$(RESET) Detecting system environment..."
	@UNAME_S=$$(uname -s); \
	if echo "$$UNAME_S" | grep -qi "microsoft"; then \
		echo "$(YELLOW)==>$(RESET) Detected: WSL environment (Linux kernel under Windows)"; \
		echo "$(YELLOW)==>$(RESET) Treating as Windows build..."; \
		$(MAKE) windows; \
	elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "$(YELLOW)==>$(RESET) Detected: Termux environment"; \
		pkg update -y && pkg install -y clang openssl curl libarchive ncurses readline; \
		$(MAKE) termux; \
	elif echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "$(YELLOW)==>$(RESET) Detected: MSYS2 MinGW UCRT64 environment"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			base-devel \
			mingw-w64-ucrt-x86_64-clang \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-ncurses \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-openssl \
			mingw-w64-ucrt-x86_64-upx; \
		$(MAKE) windows; \
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "$(YELLOW)==>$(RESET) Detected: Native Linux environment"; \
		sudo dpkg --add-architecture i386; \
		sudo apt update -y && \
        sudo apt install -y build-essential \
			clang \
			lld \
			libc++-dev \
			libc++abi-dev \
			make \
			cmake \
			ninja-build \
			libssl-dev \
			libncurses5-dev \
			libncursesw5-dev \
			libcurl4-openssl-dev \
			libreadline-dev \
			libarchive-dev \
			zlib1g-dev \
			libonig-dev \
            xterm; \
		$(MAKE) linux; \
	else \
		echo "$(YELLOW)==>$(RESET) Unknown or unsupported environment."; \
		exit 1; \
	fi

all: $(TARGET)
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
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
		$(STRIP) --strip-all $(TARGET) || true; \
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
	rm -f $(OBJS) watchdogs watchdogs.tmux watchdogs.win watchdogs.debug watchdogs.debug.win
	@printf "$(YELLOW)==>$(RESET) Clean done.\n"

linux:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -o $(TARGET) $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"

termux:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building Termux target with clang...\n"
	$(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -pie
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.tmux Version $(VERSION) Full Version $(FULL_VERSION)\n"

windows:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building Windows target (MinGW)...\n"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) -o watchdogs.win $(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.win Version $(VERSION) Full Version $(FULL_VERSION)\n"

debug:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.debug Version $(VERSION) Full Version $(FULL_VERSION)\n"

termux-debug:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) I/data/data/com.termux/files/usr/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug.tmux $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.debug.tmux Version $(VERSION) Full Version $(FULL_VERSION)\n"

windows-debug:
	@echo "LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug.win $(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Debug build complete: ./watchdogs.debug.win\n"