#include "http.h"

extern char str1[60];
extern char tmpbuf[30];
extern uint8_t sect[515];
http_sock_prop_ptr httpsockprop[2];
extern tcp_prop_ptr tcpprop;

volatile uint16_t tcp_size_wnd = 2048;

const char http_header[] = {"HTTP/1.1 200 OKrnContent-Type: text/htmlrnrn"};
const char jpg_header[] = {"HTTP/1.0 200 OKrnServer: nginxrnContent-Type: image/jpegrnConnection: closernrn"};
const char icon_header[] = {"HTTP/1.1 200 OKrnContent-Type: image/x-iconrnrn"};
const char error_header[] = {"HTTP/1.0 404 File not foundrnServer: nginxrnContent-Type: text/htmlrnConnection: closernrn"};
char *header;

const uint8_t e404_htm[]
	= {0x3c, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0a, 0x20, 0x20, 0x3c, 0x68, 0x65, 0x61, 0x64, 0x3e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x74, 0x69, 0x74, 0x6c,
	   0x65, 0x3e, 0x34, 0x30, 0x34, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64, 0x3c, 0x2f, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x3e, 0x0a, 0x20,
	   0x20, 0x3c, 0x2f, 0x68, 0x65, 0x61, 0x64, 0x3e, 0x0a, 0x3c, 0x62, 0x6f, 0x64, 0x79, 0x3e, 0x0a, 0x3c, 0x68, 0x31, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65,
	   0x3d, 0x22, 0x74, 0x65, 0x78, 0x74, 0x2d, 0x61, 0x6c, 0x69, 0x67, 0x6e, 0x3a, 0x20, 0x63, 0x65, 0x6e, 0x74, 0x65, 0x72, 0x3b, 0x22, 0x3e, 0x34, 0x30,
	   0x34, 0x20, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x20, 0x46, 0x69, 0x6c, 0x65, 0x20, 0x4e, 0x6f, 0x74, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64, 0x3c, 0x2f, 0x68,
	   0x31, 0x3e, 0x0a, 0x3c, 0x68, 0x32, 0x20, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3d, 0x22, 0x74, 0x65, 0x78, 0x74, 0x2d, 0x61, 0x6c, 0x69, 0x67, 0x6e, 0x3a,
	   0x20, 0x63, 0x65, 0x6e, 0x74, 0x65, 0x72, 0x3b, 0x22, 0x3e, 0x20, 0x54, 0x68, 0x65, 0x20, 0x70, 0x61, 0x67, 0x65, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x61,
	   0x72, 0x65, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x69, 0x6e, 0x67, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x6d, 0x69, 0x67, 0x68, 0x74, 0x20, 0x68, 0x61, 0x76, 0x65,
	   0x20, 0x62, 0x65, 0x65, 0x6e, 0x20, 0x72, 0x65, 0x6d, 0x6f, 0x76, 0x65, 0x64, 0x2c, 0x20, 0x3c, 0x62, 0x72, 0x20, 0x2f, 0x3e, 0x68, 0x61, 0x64, 0x20,
	   0x69, 0x74, 0x73, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x20, 0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x2c, 0x20, 0x6f, 0x72, 0x20, 0x69, 0x73, 0x20, 0x74,
	   0x65, 0x6d, 0x70, 0x6f, 0x72, 0x61, 0x72, 0x69, 0x6c, 0x79, 0x20, 0x75, 0x6e, 0x61, 0x76, 0x61, 0x69, 0x6c, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x3c, 0x2f,
	   0x68, 0x32, 0x3e, 0x0a, 0x3c, 0x2f, 0x62, 0x6f, 0x64, 0x79, 0x3e, 0x3c, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3e};

void tcp_send_http_one(void) {
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

		//data_len = (uint16_t)MyFile.fsize;

		end_point = GetWritePointer(tcpprop.cur_sock);
		end_point += header_len + data_len;
		// Fill with data packet transmit buff
		SetWritePointer(tcpprop.cur_sock, end_point);
		end_point = GetWritePointer(tcpprop.cur_sock);
		memcpy(sect + 3, header, header_len); // 3 service bytes
		w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, header_len);
		end_point += header_len;

		num_sect = data_len / 512;

		// for (i = 0; i <= num_sect; i++) {
		// 	//не последний сектор
		// 	if (i < (num_sect - 1)) {
		// 		len_sect = 512;
		// 	} else {
		// 		len_sect = data_len;
		// 	}
		// 	result = f_lseek(&MyFile, i * 512); //Установим курсор чтения в файле		//reading 512 byte sectors from SD and writing them to tx buff
		// 	sprintf(str1, "f_lseek: %drn", result);										//modify for using my own pages
		// 	HAL_UART_Transmit(&huart2, (uint8_t *)str1, strlen(str1), 0x1000);
		// 	result = f_read(&MyFile, sect + 3, len_sect, (UINT *)&bytesread);
		// 	w5500_writeSockBuf(tcpprop.cur_sock, end_point, (uint8_t *)sect, len_sect);
		// 	end_point += len_sect;
		// 	data_len -= len_sect;
		// }
	} else {
		header_len = strlen(error_header);
		data_len = sizeof(e404_htm);
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
	SendSocket(tcpprop.cur_sock);
	httpsockprop[tcpprop.cur_sock].data_stat = DATA_COMPLETED;
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

		// HAL_UART_Transmit(&huart2, (uint8_t *)httpsockprop[tcpprop.cur_sock].fname, strlen(httpsockprop[tcpprop.cur_sock].fname), 0x1000);
		// HAL_UART_Transmit(&huart2, (uint8_t *)"rn", 2, 0x1000);
		ST7735_WriteSerialStrings((char *)httpsockprop[tcpprop.cur_sock].fname, Font_7x10, ST7735_WHITE, ST7735_BLACK);
		httpsockprop[tcpprop.cur_sock].http_doc = E404_HTML;
		// first include header size
		httpsockprop[tcpprop.cur_sock].data_size = strlen(error_header);
		// then size of document itself
		httpsockprop[tcpprop.cur_sock].data_size += sizeof(e404_htm);

		// calc number of windows
		httpsockprop[tcpprop.cur_sock].cnt_rem_data_part = httpsockprop[tcpprop.cur_sock].data_size / tcp_size_wnd + 1;
		httpsockprop[tcpprop.cur_sock].last_data_part_size = httpsockprop[tcpprop.cur_sock].data_size % tcp_size_wnd;
		// in case of wrong calc, when total size divides by min window size without remainder
		if (httpsockprop[tcpprop.cur_sock].last_data_part_size == 0) {
			httpsockprop[tcpprop.cur_sock].last_data_part_size = tcp_size_wnd;
			httpsockprop[tcpprop.cur_sock].cnt_rem_data_part--;
		}
		httpsockprop[tcpprop.cur_sock].cnt_data_part = httpsockprop[tcpprop.cur_sock].cnt_rem_data_part;
		sprintf(str1, "data size:%lu; cnt data part:%u; last_data_part_size:%urn", (unsigned long)httpsockprop[tcpprop.cur_sock].data_size,
				httpsockprop[tcpprop.cur_sock].cnt_rem_data_part, httpsockprop[tcpprop.cur_sock].last_data_part_size);
		// HAL_UART_Transmit(&huart2, (uint8_t *)str1, strlen(str1), 0x1000);
		ST7735_WriteSerialStrings(str1, Font_7x10, ST7735_WHITE, ST7735_BLACK);

		if (httpsockprop[tcpprop.cur_sock].cnt_rem_data_part == 1) {
			httpsockprop[tcpprop.cur_sock].data_stat = DATA_ONE;
		} else if (httpsockprop[tcpprop.cur_sock].cnt_rem_data_part > 1) {
			httpsockprop[tcpprop.cur_sock].data_stat = DATA_FIRST;
		}
		if (httpsockprop[tcpprop.cur_sock].data_stat == DATA_ONE) {
			tcp_send_http_one();
			DisconnectSocket(tcpprop.cur_sock); // Disconnecting
			SocketClosedWait(tcpprop.cur_sock);
			OpenSocket(tcpprop.cur_sock, Mode_TCP);
			// Waiting init socket (status SOCK_INIT)
			SocketInitWait(tcpprop.cur_sock);
			// Continue listering socket
			ListenSocket(tcpprop.cur_sock);
			SocketListenWait(tcpprop.cur_sock);
			// httpsockprop[tcpprop.cur_sock].data_stat = DATA_COMPLETED;
		} else if (httpsockprop[tcpprop.cur_sock].data_stat == DATA_FIRST) {
		}
	}
}