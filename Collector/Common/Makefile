
OBJECTS = performance.o \
	  	  LogMsg.o 

SRCS = $(OBJECTS:.o=.c)
DEPENDENCY = lcd.h 				\
		     LogMsg.h 			\
		     mfc.h 				\
		     MfcDriver.h 		\
		     MfcDrvParams.h 	\
		     performance.h 		\
			 post.h				\
			 videodev2.h		\
			 videodev2_s3c.h	

GXX = arm-linux-g++
CFLAGS = -static -g -c -Os -Wall
INC = -I../Common

TARGET = app

$(TARGET) : $(OBJECTS)
		#$(GXX) -o $(TARGET) $(OBJECTS)

.c.o :
		$(GXX) $(INC) $(CFLAGS) $<

clean :
		rm -rf $(OBJECTS) $(TARGET) core

performance.o : $(DEPENDENCY) performance.c
LogMsg.o : $(DEPENDENCY) LogMsg.c

