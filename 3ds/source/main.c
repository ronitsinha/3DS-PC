#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>

Result http_post(const char* url, const char* data)
{
	Result ret=0;
	httpcContext context;
	char *newurl=NULL;
	u32 statuscode=0;
	u32 contentsize=0, readsize=0, size=0;
	u8 *buf, *lastbuf;

	printf("POSTing %s\n", url);

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
		printf("return from httpcOpenContext: %" PRIx32 "\n",ret);

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
		printf("return from httpcSetSSLOpt: %" PRIx32 "\n",ret);

		// Enable Keep-Alive connections
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		printf("return from httpcSetKeepAlive: %" PRIx32 "\n",ret);

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "httpc-example/1.0.0");
		printf("return from httpcAddRequestHeaderField: %" PRIx32 "\n",ret);

		// Set a Content-Type header so websites can identify the format of our raw body data.
		// If you want to send form data in your request, use:
		//ret = httpcAddRequestHeaderField(&context, "Content-Type", "multipart/form-data");
		// If you want to send raw JSON data in your request, use:
		ret = httpcAddRequestHeaderField(&context, "Content-Type", "text/html");
		printf("return from httpcAddRequestHeaderField: %" PRIx32 "\n",ret);

		// Post specified data.
		// If you want to add a form field to your request, use:
		//ret = httpcAddPostDataAscii(&context, "data", value);
		// If you want to add a form field containing binary data to your request, use:
		//ret = httpcAddPostDataBinary(&context, "field name", yourBinaryData, length);
		// If you want to add raw data to your request, use:
		ret = httpcAddPostDataRaw(&context, (u32*)data, strlen(data));
		printf("return from httpcAddPostDataRaw: %" PRIx32 "\n",ret);

		ret = httpcBeginRequest(&context);
		if(ret!=0){
			httpcCloseContext(&context);
			if(newurl!=NULL) free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if(ret!=0){
			httpcCloseContext(&context);
			if(newurl!=NULL) free(newurl);
			return ret;
		}

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if(newurl==NULL) newurl = malloc(0x1000); // One 4K page for new URL
			if (newurl==NULL){
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned
			printf("redirecting to url: %s\n",url);
			httpcCloseContext(&context); // Close this context before we try the next
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if(statuscode!=200){
		printf("URL returned status: %" PRIx32 "\n", statuscode);
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return -2;
	}

	// This relies on an optional Content-Length header and may be 0
	ret=httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if(ret!=0){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return ret;
	}

	printf("reported size: %" PRIx32 "\n",contentsize);

	// Start with a single page buffer
	buf = (u8*)malloc(0x1000);
	if(buf==NULL){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		return -1;
	}

	do {
		// This download loop resizes the buffer as data is read.
		ret = httpcDownloadData(&context, buf+size, 0x1000, &readsize);
		size += readsize; 
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING){
				lastbuf = buf; // Save the old pointer, in case realloc() fails.
				buf = realloc(buf, size + 0x1000);
				if(buf==NULL){ 
					httpcCloseContext(&context);
					free(lastbuf);
					if(newurl!=NULL) free(newurl);
					return -1;
				}
			}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);	

	if(ret!=0){
		httpcCloseContext(&context);
		if(newurl!=NULL) free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = realloc(buf, size);
	if(buf==NULL){ // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if(newurl!=NULL) free(newurl);
		return -1;
	}

	printf("response size: %" PRIx32 "\n",size);

	// Print result
	printf((char*)buf);
	printf("\n");
	
	gfxFlushBuffers();
	gfxSwapBuffers();

	httpcCloseContext(&context);
	free(buf);
	if (newurl!=NULL) free(newurl);

	return 0;
}

int main()
{
	Result ret=0;
	gfxInitDefault();
	httpcInit(4 * 1024 * 1024); // Buffer size when POST/PUT.

	consoleInit(GFX_BOTTOM,NULL);

	ret=http_post("http://192.168.1.197:8080", "{\"foo\": \"bar\"}");
	printf("return from http_post: %" PRIx32 "\n",ret);

	circlePosition oldPos;

	char posStr[11];

	u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp = hidKeysUp();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu


		//Do the keys printing only if keys have changed
		if (kDown != kDownOld || kHeld != kHeldOld || kUp != kUpOld)
		{
			if (kDown & KEY_A) ret=http_post("http://192.168.1.197:8080", "KEY_A DOWN");
			if (kHeld & KEY_A) ret=http_post("http://192.168.1.197:8080", "KEY_A HELD");
			if (kUp & KEY_A) ret=http_post("http://192.168.1.197:8080", "KEY_A UP"); 
		}

		kDownOld = kDown;
		kHeldOld = kHeld;
		kUpOld = kUp;

		circlePosition pos;

		//Read the CirclePad position
		hidCircleRead(&pos);

		//Print the CirclePad position
		printf("\x1b[3;1H%04d; %04d", pos.dx, pos.dy);
		
		// if (oldPos.dx != pos.dx || oldPos.dy != pos.dy) {
		char posX[5];
		char posY[5];

		sprintf(posX, "%d", pos.dx);
		sprintf(posY, "%d", pos.dy);

		strcat(posStr, posX);
		strcat(posStr, " ");
		strcat(posStr, posY);

		ret=http_post("http://192.168.1.197:8080", posStr);
		printf("return from http_post: %" PRIx32 "\n",ret);
		printf(posX);

		memset(posStr, 0, sizeof posStr);
		// }

		oldPos = pos;

		gspWaitForVBlank();
	}

	// Exit services
	httpcExit();
	gfxExit();
	return 0;
}

