CC				= gcc
CFLAGS			= -Wall

RELEXE			= matrix
INSTALL_PATH	= /usr/local/bin
SHARE_PATH		= /usr/share/matrix
APP				= /usr/share/applications
MIME			= /usr/share/mime
ICON			= /usr/share/icons/hicolor/32x32/apps/
LOCAL			= locale/ru/LC_MESSAGES/matrix

OBJS			= main.o utils.o mafile.o dialog.o paint.o puzzle.o print.o library.o lzw.o
SRCS			= $(OBJS:.c=.o)

DCFLAGS			= -g -O0 -DDEBUG `pkg-config --cflags gtk+-2.0` `cups-config --cflags`
DOFLAGS			= `pkg-config --libs gtk+-2.0 libnotify` `cups-config --libs` -lssl -lcrypto -lpthread

RCFLAGS			= -O3 -DNDEBUG `pkg-config --cflags gtk+-2.0` `cups-config --cflags`
ROFLAGS			= -s -pipe `pkg-config --libs gtk+-2.0 libnotify` `cups-config --libs` -lssl -lcrypto -lpthread

.PHONY: all clean install uninstall

all: $(RELEXE)

$(RELEXE):	$(OBJS)
			msgfmt $(LOCAL).po -o $(LOCAL).mo
			$(CC) $(CFLAGS) $^ -o $(RELEXE) $(ROFLAGS)

%.o: %.c
			$(CC) -c $(CFLAGS) $(RCFLAGS) -o $@ $<

#debug: $(DBGEXE)

#$(DBGEXE): $(OBJS)
#			msgfmt $(LOCAL).po -o $(LOCAL).mo
#			$(CC) $(CFLAGS) $(DOFLAGS) -o $(DBGEXE) $^

#%.o: %.c
#			$(CC) -c $(CFLAGS) $(DCFLAGS) -o $@ $<

clean:
			rm -rf $(OBJS) $(RELEXE) *.gch

install:
			install ./$(RELEXE) $(INSTALL_PATH)
			mkdir -p $(SHARE_PATH)/res
#			mkdir -p $(SHARE_PATH)/example
			install res/* $(SHARE_PATH)/res
			install res/matrix.png $(ICON)
			install matrix.desktop $(APP)
			install locale/ru/LC_MESSAGES/*.mo /usr/share/locale/ru/LC_MESSAGES
			xdg-mime install --mode system application-matrix.xml

uninstall:
			rm -rf $(INSTALL_PATH)/$(RELEXE) $(APP)/matrix.desktop $(SHARE_PATH) $(MIME)/application/matrix.xml
			rm /usr/share/locale/ru/LC_MESSAGES/matrix.mo
			xdg-mime uninstall --mode system application-matrix.xml
