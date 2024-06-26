MAIN_OBJ=src/cornellbox.cc
OBJS:=$(filter-out $(MAIN_OBJ), $(wildcard src/*.cc))
TEST_OBJS:=$(wildcard test/*.cc)

CC?=$(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,clang++)

COMPILER_FLAGS=-std=c++17
TEST_COMPILER_FLAGS=-g -DTEST
LIVE_COMPILER_FLAGS=-O3

COMPILER_FLAGS += ${CUSTOM_FLAGS}

LINKER_FLAGS=-lSDL -lSDL_ttf -lpthread -lm -lstdc++
LINKER_FLAGS_2=-lSDL2 -lSDL2_ttf -lpthread -lm -lstdc++

OBJ_NAME=build/cornellbox
TEST_OBJ_NAME=build/cornellbox_test

.PHONY: all
.PHONY: miyoo
.PHONY: miyooa30
.PHONY: rg35xx
.PHONY: rg35xxhf
.PHONY: arm64
.PHONY: test
.PHONY: run

all: $(MAIN_OBJ) $(OBJS)
	mkdir -p build
	$(CC) -DBASIC_VECTORS $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output $(OBJ_NAME)

build/cb2: $(MAIN_OBJ) $(OBJS)
	mkdir -p build
	$(CC) -DBASIC_VECTORS -DUSE_SDL2 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS_2) \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/cb2

build/miyoo/cornellbox: $(MAIN_OBJ) $(OBJS)
	mkdir -p build/miyoo
	$(CC) -DFLIP_SCREEN -DBASIC_VECTORS -DBASIC_RENDERER -O3 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		-funsafe-math-optimizations -mfpu=neon \
		-fopt-info-vec-optimized \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/miyoo/cornellbox

build/miyoo/assets: assets
	cp -ru assets build/miyoo/

build/miyoo/config.json: assets
	cp -u miyoo/config.json build/miyoo/

miyoo: build/miyoo/cornellbox build/miyoo/assets build/miyoo/config.json

build/rg35xx/cornellbox: $(MAIN_OBJ) $(OBJS)
	mkdir -p build/rg35xx
	$(CC) -DRED_BLUE_SWAP -DBASIC_VECTORS -DBASIC_RENDERER -O3 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		-march=armv7-a -mfloat-abi=softfp \
		-funsafe-math-optimizations -mfpu=neon \
		-fopt-info-vec-optimized \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/rg35xx/cornellbox

build/rg35xx/assets: assets
	cp -ru assets build/rg35xx/

build/rg35xxhf/cornellbox: $(MAIN_OBJ) $(OBJS)
	mkdir -p build/rg35xxhf
	$(CC) -DBASIC_VECTORS -DBASIC_RENDERER -O3 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		-funsafe-math-optimizations -mfpu=neon \
		-fopt-info-vec-optimized \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/rg35xxhf/cornellbox

build/rg35xxhf/assets: assets
	cp -ru assets build/rg35xxhf/

build/miyooa30/cornellbox: $(MAIN_OBJ) $(OBJS)
	mkdir -p build/miyooa30
	$(CC) -DBASIC_VECTORS -DBASIC_RENDERER -DPORTRAIT -DUSE_SDL2 -O3 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS_2) \
		-funsafe-math-optimizations -mfpu=neon \
		-fopt-info-vec-optimized \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/miyooa30/cornellbox

build/miyooa30/assets: assets
	cp -ru assets build/miyooa30/

build/miyooa30/config.json: assets
	cp -u miyoo/config.json build/miyooa30/

build/arm64/cornellbox: $(MAIN_OBJ) $(OBJS)
	mkdir -p build/arm64
	$(CC) -DBASIC_VECTORS -DBASIC_RENDERER -O3 $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		-funsafe-math-optimizations \
		-fopt-info-vec-optimized \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output build/arm64/cornellbox

build/arm64/assets: assets
	cp -ru assets build/arm64/

rg35xx: build/rg35xx/cornellbox build/rg35xx/assets

rg35xxhf: build/rg35xxhf/cornellbox build/rg35xxhf/assets

miyooa30: build/miyooa30/cornellbox build/miyooa30/assets build/miyooa30/config.json

arm64: build/arm64/cornellbox build/arm64/assets

$(TEST_OBJ_NAME): $(TEST_OBJS) $(OBJS)
	mkdir -p build
	$(CC) $(TEST_OBJS) $(OBJS) $(LINKER_FLAGS) \
		$(COMPILER_FLAGS) $(TEST_COMPILER_FLAGS) --output $(TEST_OBJ_NAME)

test: $(TEST_OBJ_NAME) $(TEST_OBJS) $(OBJS)
	$(TEST_OBJ_NAME)

run: $(OBJ_NAME) $(MAIN_OBJ) $(OBJS)
	$(OBJ_NAME)
