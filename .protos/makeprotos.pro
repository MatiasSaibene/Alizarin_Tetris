") + 8;
       (long)seek > 8;
       seek = strstr(seek, "*PROTO*/") + 8)
   ; at character %d\n",
		  seek - text);
	  exit(1);
      }
      end --;
      memcpy(output,seek,end-seek);
      output += end-seek+1;       
      output[-1]=';';
      seek = end+2;
    };

  munmap(text,buf.st_size); 

  *output++ = '\n';
  *output = 0; 
  output_size = output - output_file;
     
  fdold = open(newname, O_RDONLY);
  
  if (fdold <= 0);
