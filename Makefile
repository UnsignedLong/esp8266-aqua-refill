PFIO = platformio


default: build

all: clean upload

build: src/main.ino
	${PFIO} run

upload: build
	${PFIO} run -t upload

clean:
	${PFIO} run -t clean