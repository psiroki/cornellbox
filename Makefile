MAIN_OBJ=src/cornellbox.cc
OBJS:=$(filter-out $(MAIN_OBJ), $(wildcard src/*.cc))
TEST_OBJS:=$(wildcard test/*.cc)

CC?=$(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,clang++)

COMPILER_FLAGS=-std=c++17 -I/usr/include/SDL
TEST_COMPILER_FLAGS=-g -DTEST
LIVE_COMPILER_FLAGS=-O3

LINKER_FLAGS=-lSDL -lSDL_ttf -lpthread -lm -lstdc++

OBJ_NAME=build/cornellbox
TEST_OBJ_NAME=build/cornellbox_test

.PHONY: all
.PHONY: miyoo
.PHONY: rg35xx
.PHONY: rg35xxhf
.PHONY: test
.PHONY: run

all: $(MAIN_OBJ) $(OBJS)
	mkdir -p build
	$(CC) -DBASIC_VECTORS $(MAIN_OBJ) $(OBJS) $(LINKER_FLAGS) \
		$(COMPILER_FLAGS) $(LIVE_COMPILER_FLAGS) --output $(OBJ_NAME)

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

rg35xx: build/rg35xx/cornellbox build/rg35xx/assets

rg35xxhf: build/rg35xxhf/cornellbox build/rg35xxhf/assets

$(TEST_OBJ_NAME): $(TEST_OBJS) $(OBJS)
	mkdir -p build
	$(CC) $(TEST_OBJS) $(OBJS) $(LINKER_FLAGS) \
		$(COMPILER_FLAGS) $(TEST_COMPILER_FLAGS) --output $(TEST_OBJ_NAME)

test: $(TEST_OBJ_NAME) $(TEST_OBJS) $(OBJS)
	$(TEST_OBJ_NAME)

run: $(OBJ_NAME) $(MAIN_OBJ) $(OBJS)
	$(OBJ_NAME)
