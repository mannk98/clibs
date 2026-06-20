################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ringbuffchar.c \
../src/str_utils.c 

C_DEPS += \
./src/ringbuffchar.d \
./src/str_utils.d 

OBJS += \
./src/ringbuffchar.o \
./src/str_utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -Os -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/ringbuffchar.d ./src/ringbuffchar.o ./src/str_utils.d ./src/str_utils.o

.PHONY: clean-src

