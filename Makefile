# Makefile for virtual memory program
CC=gcc
CFLAGS=-Wall

all: vm

vm: vm.o
	$(CC) $(CFLAGS) -o vm vm.o
