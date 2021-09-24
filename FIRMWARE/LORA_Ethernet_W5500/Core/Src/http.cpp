#include "http.h"
#include "web_pages.h"
#include <map>
#include <string>

#define PAGES_NUM 2

extern char str1[60];
extern char tmpbuf[30];
extern uint8_t sect[515];
http_sock_prop_ptr httpsockprop[2];
extern tcp_prop_ptr tcpprop;

bool is_page_exist = false;
volatile uint16_t tcp_size_wnd = 2048;

char *pages_names[] = {"hello_world.html", "houston.html"};
std::map<std::string, char *> pages_map = {{pages_names[0], hello_world_page}, {pages_names[1], houston}};

const char http_header[] = {"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"};
const char jpg_header[] = {"HTTP/1.0 200 OK\r\nServer: nginx\r\nContent-Type: image/jpeg\r\nConnection: close\r\n\r\n"};
const char icon_header[] = {"HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\n\r\n"};
const char error_header[] = {"HTTP/1.0 404 File not found\r\nServer: nginx\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"};
char *header;

bool in_array(char *value, char *array[]) {
	int size = PAGES_NUM;
	for (int i = 0; i < size; i++) {
		if (!strcmp(value, array[i])) {
			return true;
		}
	}
	return false;
}

void tcp_send_http_one_data() {
	uint16_t i = 0;
	uint16_t data_len = 0;
	uint16_t header_len = 0;
	uint16_t end_point;
	uint8_t num_sect = 0;
	uint16_t len_sect;

	if ((httpsockprop[tcpprop.cur_sock].http_doc == EXISTING_HTML) || (httpsockprop[tcpprop.cur_sock].http_doc == EXISTING_JPG)
		|| (httpsockprop[tcpprop.cur_sock].http_doc == EXISTING_ICO)) {

		switch (httpsockprop[tcpprop.cur_sock].http_doc) {
			case EXISTING_HTML:
				header = (char *)http_header;
				break;
			case EXISTING_ICO:
				header = (char *)icon_header;
				break;
			case EXISTING_JPG:
				header = (char *)jpg_header;
				break;
		}
		header_len = strlen(header);

		data_len = strlen(pages_map[httpsockprop[tcpprop.cur_sock].fname]);

		end_point = GetWritePointer(tcpprop.cur_sock);
		end_point += header_len + data_len;
		// Fill with data packet transmit buff
		SetWritePointer(tcpprop.cur_sock, end_point);
		end_point = GetWritePointer(tcpprop.cur_sock);
		memcpy(sect + 3, header, header_len); // 3 service bytes
		w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, header_len);
		end_point += header_len;

		num_sect = data_len / 512;

		for (i = 0; i <= num_sect; i++) {
			// not last sector
			if (i < (num_sect - 1)) {
				len_sect = 512;
			} else {
				len_sect = data_len;
			}

			memcpy(sect + 3, pages_map[httpsockprop[tcpprop.cur_sock].fname] + i * 512, 512); // reading 512 byte sectors from map and writing them to tx buff

			w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, len_sect);
			end_point += len_sect;
			data_len -= len_sect;
		}
	} else { // error page case
		header_len = strlen(error_header);
		data_len = strlen(e404_htm);
		end_point = GetWritePointer(tcpprop.cur_sock);
		end_point += header_len + data_len;
		SetWritePointer(tcpprop.cur_sock, end_point);
		end_point = GetWritePointer(tcpprop.cur_sock);

		// Fill with data packet transmit buff
		memcpy(sect + 3, error_header, header_len);
		w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, header_len);
		end_point += header_len;
		memcpy(sect + 3, e404_htm, data_len);
		w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, data_len);
		end_point += data_len;
	}

	RecvSocket(tcpprop.cur_sock);
	SendSocket(tcpprop.cur_sock); // sending http answer
	httpsockprop[tcpprop.cur_sock].data_stat = DATA_COMPLETED;
}

void tcp_send_http_first_prt_data() {
	uint8_t prt;
	uint16_t i = 0;
	uint16_t data_len = 0;
	uint16_t header_len = 0;
	uint16_t end_point;
	uint8_t num_sect = 0;
	uint16_t len_sect;
	uint16_t last_part, last_part_size;

	// Don't check if file exist, because of error404 html page fits in only one buffer
	switch (httpsockprop[tcpprop.cur_sock].http_doc) {
		case EXISTING_HTML:
			header = (char *)http_header;
			break;
		case EXISTING_ICO:
			header = (char *)icon_header;
			break;
		case EXISTING_JPG:
			header = (char *)jpg_header;
			break;
	}
	header_len = strlen(header);
	data_len = tcp_size_wnd - header_len;

	end_point = GetWritePointer(tcpprop.cur_sock); // Define and set pointer in transmit buffer for writing
	end_point += header_len + data_len;
	SetWritePointer(tcpprop.cur_sock, end_point);
	end_point = GetWritePointer(tcpprop.cur_sock);

	//--------------------------------------------------------------------------
	// some sort of strange code
	// Narod stream: "let's save some vars, because they are lost for some reason
	last_part = httpsockprop[tcpprop.cur_sock].cnt_rem_data_part;
	last_part_size = httpsockprop[tcpprop.cur_sock].last_data_part_size;
	prt = httpsockprop[tcpprop.cur_sock].prt_tp;
	//--------------------------------------------------------------------------

	// Fill with data packet transmit buffer
	memcpy(sect + 3, header, header_len);
	w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, header_len);
	end_point += header_len;
	num_sect = data_len / 512;

	for (i = 0; i <= num_sect; i++) {
		//не последний сектор
		if (i < (num_sect - 1)) {
			len_sect = 512;
		} else {
			len_sect = data_len;
		}

		memcpy(sect + 3, pages_map[httpsockprop[tcpprop.cur_sock].fname] + i * 512, 512); // reading 512 byte sectors from map and writing them to tx buff

		w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, len_sect);
		end_point += len_sect;
		data_len -= len_sect;
	}

	//--------------------------------------------------------------------------
	// Continuation of some sort of strange code
	// Narod stream: "return params"
	httpsockprop[tcpprop.cur_sock].cnt_rem_data_part = last_part;
	httpsockprop[tcpprop.cur_sock].last_data_part_size = last_part_size;
	httpsockprop[tcpprop.cur_sock].prt_tp = prt;
	//--------------------------------------------------------------------------
	// sending http answer
	RecvSocket(tcpprop.cur_sock);
	SendSocket(tcpprop.cur_sock);
	// we have send one part of data, so must decrement remain num of parts
	httpsockprop[tcpprop.cur_sock].cnt_rem_data_part--;

	if (httpsockprop[tcpprop.cur_sock].cnt_rem_data_part > 1) {
		httpsockprop[tcpprop.cur_sock].data_stat = DATA_MIDDLE;
	} else {
		httpsockprop[tcpprop.cur_sock].data_stat = DATA_LAST;
	}

	// num of transmitted bytes
	httpsockprop[tcpprop.cur_sock].total_count_bytes = tcp_size_wnd - header_len;
}

void http_request() {
	uint16_t point;
	uint8_t RXbyte;
	uint16_t i = 0;
	char *ss1;
	int ch1 = '.';

	// finding first "/" symbol in HTTP head
	point = GetReadPointer(tcpprop.cur_sock);
	i = 0;
	while (RXbyte != (uint8_t)'/') {
		RXbyte = w5500_readSockBufByte(tcpprop.cur_sock, point + i);
		i++;
	}
	point += i;
	RXbyte = w5500_readSockBufByte(tcpprop.cur_sock, point);

	if (RXbyte == (uint8_t)' ') {
		strcpy(httpsockprop[tcpprop.cur_sock].fname, "index.html");
		httpsockprop[tcpprop.cur_sock].http_doc = EXISTING_HTML;
	} else {
		// finding next space (" ") in HTTP head, reading file name from request
		i = 0;
		while (tmpbuf[i - 1] != (uint8_t)' ') {
			tmpbuf[i] = w5500_readSockBufByte(tcpprop.cur_sock, point + i);
			i++;
		}
		i--;		   // set cntr to end of file name
		tmpbuf[i] = 0; //закончим строку
		strcpy(httpsockprop[tcpprop.cur_sock].fname, tmpbuf);
		ST7735_WriteSerialStrings((char *)httpsockprop[tcpprop.cur_sock].fname, Font_7x10, ST7735_WHITE, ST7735_BLACK);
		is_page_exist = in_array(httpsockprop[tcpprop.cur_sock].fname, pages_names);

		if (is_page_exist) {
			// discover file extension
			ss1 = strchr(httpsockprop[tcpprop.cur_sock].fname, ch1);
			ss1++;
			if (strncmp(ss1, "jpg", 3) == 0) {
				httpsockprop[tcpprop.cur_sock].http_doc = EXISTING_JPG;
				// first include header size
				httpsockprop[tcpprop.cur_sock].data_size = strlen(jpg_header);
			}
			if (strncmp(ss1, "ico", 3) == 0) {
				httpsockprop[tcpprop.cur_sock].http_doc = EXISTING_ICO;
				// first include header size
				httpsockprop[tcpprop.cur_sock].data_size = strlen(icon_header);
			} else {
				httpsockprop[tcpprop.cur_sock].http_doc = EXISTING_HTML;
				// first include header size
				httpsockprop[tcpprop.cur_sock].data_size = strlen(http_header);
			}
			// then size of document itself
			httpsockprop[tcpprop.cur_sock].data_size += strlen(pages_map[httpsockprop[tcpprop.cur_sock].fname]);
		} else {
			httpsockprop[tcpprop.cur_sock].http_doc = E404_HTML;
			// first include header size
			httpsockprop[tcpprop.cur_sock].data_size = strlen(error_header);
			// then size of document itself
			httpsockprop[tcpprop.cur_sock].data_size += strlen(e404_htm);
		}
		// calc number of windows
		httpsockprop[tcpprop.cur_sock].cnt_rem_data_part = httpsockprop[tcpprop.cur_sock].data_size / tcp_size_wnd + 1;
		httpsockprop[tcpprop.cur_sock].last_data_part_size = httpsockprop[tcpprop.cur_sock].data_size % tcp_size_wnd;
		// in case of wrong calc, when total size divides by min window size without remainder
		if (httpsockprop[tcpprop.cur_sock].last_data_part_size == 0) {
			httpsockprop[tcpprop.cur_sock].last_data_part_size = tcp_size_wnd;
			httpsockprop[tcpprop.cur_sock].cnt_rem_data_part--;
		}
		httpsockprop[tcpprop.cur_sock].cnt_data_part = httpsockprop[tcpprop.cur_sock].cnt_rem_data_part;
		sprintf(str1, "data size:%lu; cnt data part:%u; last_data_part_size:%u\r\n", (unsigned long)httpsockprop[tcpprop.cur_sock].data_size,
				httpsockprop[tcpprop.cur_sock].cnt_rem_data_part, httpsockprop[tcpprop.cur_sock].last_data_part_size);
		ST7735_WriteSerialStrings(str1, Font_7x10, ST7735_WHITE, ST7735_BLACK);

		if (httpsockprop[tcpprop.cur_sock].cnt_rem_data_part == 1) {
			httpsockprop[tcpprop.cur_sock].data_stat = DATA_ONE;
		} else if (httpsockprop[tcpprop.cur_sock].cnt_rem_data_part > 1) {
			httpsockprop[tcpprop.cur_sock].data_stat = DATA_FIRST;
		}
		if (httpsockprop[tcpprop.cur_sock].data_stat == DATA_ONE) {
			tcp_send_http_one_data();

			DisconnectSocket(tcpprop.cur_sock); // Disconnecting
			SocketClosedWait(tcpprop.cur_sock);

			OpenSocket(tcpprop.cur_sock, Mode_TCP);
			// Waiting init socket (status SOCK_INIT)
			SocketInitWait(tcpprop.cur_sock);

			// Continue listering socket
			ListenSocket(tcpprop.cur_sock);
			SocketListenWait(tcpprop.cur_sock);
		} else if (httpsockprop[tcpprop.cur_sock].data_stat == DATA_FIRST) {
			tcp_send_http_first_prt_data();
		}
	}
}