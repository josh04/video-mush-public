//
//  udpFrameBuffer.hpp
//  video-mush
//
//  Created by Visualisation on 20/03/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef _WIN32

#ifndef media_encoder_udpFrameBuffer_hpp
#define media_encoder_udpFrameBuffer_hpp
#define bzero(p, size)     (void)memset((p), 0, (size))

#include <sys/socket.h>

#include <Mush Core/frameGrabber.hpp>

class udpFrameBuffer : public mush::frameGrabber {
public:
	udpFrameBuffer() : mush::frameGrabber(mush::inputEngine::udpInput) {
		
	}
	
	~udpFrameBuffer() {
		
	}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		_width = config.inputWidth;
		_height = config.inputHeight;
		_size = 1;
		
		if (_width * _height > 0) {
			for (int i = 0; i < config.inputBuffers; i++) {
				addItem(context->hostWriteBuffer(_width * _height * _size * 4));
			}
            
		}
		
	}
	
	void gather() {
		
		unsigned char * line;
		
		int fd;
		struct sockaddr_in servaddr,my_addr;
		
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create socket");
			return;
		}
		
		printf("created socket: descriptor=%d\n", fd);
		
		/* host byte order */
		bzero(&my_addr,sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		/* short, network byte order */
		my_addr.sin_port = htons(2326);
		/* automatically fill with my IP */
		my_addr.sin_addr.s_addr = INADDR_ANY;
		//printf("myaddr: 5X\n",my_addr.sin_addr.s_addr);
        //string s = "sss";
		if(bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
		{
			perror("Server-bind() error lol!");
			exit(1);
		} else {
			printf("Server-bind() is OK...\n");
		}
			
		socklen_t addrlen = sizeof(servaddr);
		int len = 0;
		//printf("%d\n",len);
		while(1){
			line = (unsigned char *)inLock().get_pointer();
			int c=0;
			while (1){
				len = recv(fd, (char *) line, 1280, 0);
				//printf("%d\n",len);
				if(len != 1280) break;
				c++;
			}
			
			for( int i = 0; i < 720*2*1280;i += len) {
				if ((len = recv(fd, (char *) line + i, 1280, 0)) == -1){
					perror("recv()");
					exit(1);
				}
			}
			inUnlock();
			line = nullptr;
		}

	}
protected:
private:
};

#endif

#endif
