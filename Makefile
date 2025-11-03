UNAME_S := $(shell uname -s)

export TERM
YELLOW := $(shell if command -v tput >/dev/null 2>&1; then tput setaf 3; else printf '\033[1;33m'; fi)
RESET  := $(shell if command -v tput >/dev/null 2>&1; then tput sgr0; else printf '\033[0m'; fi)
MAKE_TERMOUT := 1

export LANG := C.UTF-8
export LC_ALL := C.UTF-8

VERSION  	 = WD-11.01
FULL_VERSION = WD-251101
TARGET   	 ?= watchdogs
CC       	 ?= clang
STRIP        ?= llvm-strip

CFLAGS   = -Os -pipe -s -fdata-sections -ffunction-sections
LDFLAGS  = -Wl,-O0,--gc-sections -lm -lcurl -lreadline -lncursesw -larchive

SRCS = color.c curl.c chain.c utils.c depends.c hardware.c compiler.c archive.c package.c server.c crypto.c \
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
		pkg update -y && pkg install -y unstable-repo coreutils procps clang curl libarchive ncurses readline; \
		$(MAKE) termux; \
	elif echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "$(YELLOW)==>$(RESET) Detected: MSYS2 MinGW UCRT64 environment"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			clang \
			base-devel \
			mingw-w64-ucrt-x86_64-gcc \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-ncurses \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-upx; \
		$(MAKE) windows; \
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "$(YELLOW)==>$(RESET) Detected: Native Linux environment"; \
		echo "$(YELLOW)==>$(RESET) Installing dependencies without sudo..."; \
		if command -v apt >/dev/null 2>&1; then \
			apt update -y && \
			apt install -y build-essential \
				procps \
				clang \
				lld \
				make \
				libncursesw5-dev \
				libreadline-dev \
				libarchive-dev \
				zlib1g-dev \
				libonig-dev \
				xterm; \
		elif command -v yum >/dev/null 2>&1; then \
			yum groupinstall -y "Development Tools" && \
			yum install -y clang lld libcxx-devel-devel ncurses-devel curl-devel readline-devel libarchive-devel zlib-devel oniguruma-devel xterm; \
		elif command -v dnf >/dev/null 2>&1; then \
			dnf groupinstall -y "Development Tools" && \
			dnf install -y clang lld libcxx-devel-devel ncurses-devel curl-devel readline-devel libarchive-devel zlib-devel oniguruma-devel xterm; \
		elif command -v pacman >/dev/null 2>&1; then \
			pacman -Sy --noconfirm && \
			pacman -S --needed --noconfirm base-devel clang lld libc++ ncurses curl readline libarchive zlib oniguruma xterm; \
		else \
			echo "$(RED)==>$(RESET) Cannot install dependencies: package manager not found"; \
			echo "$(YELLOW)==>$(RESET) Please install dependencies manually"; \
		fi; \
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
	@printf "\n$(YELLOW)==>$(RESET) Clean done.\n"

linux:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -o $(TARGET) $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"

termux:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building Termux target with clang...\n"
	$(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$PREFIX/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -pie
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.tmux Version $(VERSION) Full Version $(FULL_VERSION)\n"

windows:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building Windows target (MinGW)...\n"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) -o watchdogs.win $(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.win Version $(VERSION) Full Version $(FULL_VERSION)\n"

debug:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.debug Version $(VERSION) Full Version $(FULL_VERSION)\n"

termux-debug:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$PREFIX/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS)
	@printf "$(YELLOW)==>$(RESET) Build complete: watchdogs.debug.tmux Version $(VERSION) Full Version $(FULL_VERSION)\n"

windows-debug:
	@echo "-> LANG = $$LANG"
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug.win $(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Debug build complete: ./watchdogs.debug.win\n"