    for (int i = 0; format[i];) {
        if (format[i] != '%') {
            addchar(format[i]);
            i++;
            continue;
        }
        i++;
        
        char left_pad = ' ';
        if (format[i] == '0') { left_pad = '0'; i++; }
        int width = 0;
        if (format[i] == '*') {
            width = va_arg(args, int);
            i++;
        } else {
            while (format[i] >= '0' && format[i] <= '9') {
                width = width * 10 + format[i] - '0';
                i++;
            }
        }

        int h = 0, l = 0, z = 0;
        while (format[i] == 'h') h++, i++;
        while (format[i] == 'l') l++, i++;
        while (format[i] == 'z') z++, i++;
        
        char spec = format[i];
        i++;
        switch (spec) {
            case 'c':
                for (int i = 1; i < width; i++) {
                    addchar(left_pad);
                }
                // ignoring %lc
                addchar((char) va_arg(args, int));
                break;
            case 's': {
                // ignoring %ls
                char *s = va_arg(args, char*);
                for (int i = strlen(s); i < width; i++) {
                    addchar(left_pad);
                }
                while (*s) {
                    addchar(*s);
                    s++;
                }
            }   break;
            case '%':
                for (int i = 1; i < width; i++) {
                    addchar(left_pad);
                }
                addchar('%');
                break;
            case 'n':
                     if (h >= 2) *va_arg(args, signed char*)   = printed;
                else if (h == 1) *va_arg(args, short int*)     = printed;
                else if (l == 1) *va_arg(args, long int*)      = printed;
                else if (l >= 2) *va_arg(args, long long int*) = printed;
                else if (z >= 1) *va_arg(args, size_t*)        = printed;
                else             *va_arg(args, int*)           = printed;
                break;
            case 'd':
            case 'i': // signed decimal integer
            case 'u': // unsigned decimal integer
            case 'o': // unsigned octal
            case 'x': // unsigned hex
            case 'X': // unsigned hex
            case 'p': { // unsigned hex, lowercase
                uint64_t data;
                if (spec == 'd' || spec == 'i') {
                         if (h >= 2) data = (signed char) va_arg(args, int);
                    else if (h == 1) data = (short int)   va_arg(args, int);
                    else if (l == 1) data = va_arg(args, long int);
                    else if (l >= 2) data = va_arg(args, long long int);
                    else if (z >= 1) data = va_arg(args, size_t);
                    else             data = va_arg(args, int);
                } else if (spec == 'u' || spec == 'o' || spec == 'x' || spec == 'X') {
                         if (h >= 2) data = (unsigned char)      va_arg(args, int);
                    else if (h == 1) data = (unsigned short int) va_arg(args, int);
                    else if (l == 1) data = va_arg(args, unsigned long int);
                    else if (l >= 2) data = va_arg(args, unsigned long long int);
                    else if (z >= 1) data = va_arg(args, size_t);
                    else             data = va_arg(args, unsigned int);
                } else if (spec == 'p') {
                    data = (uint64_t) va_arg(args, void*);
                }
                char sign = 0;
                if (spec == 'd' || spec == 'i') {
                    int64_t signed_data = data;
                    if (signed_data < 0) {
                        sign = '-';
                        data = -data;
                    }
                }
                
                int base = 10;
                if (spec == 'o') base = 8;
                if (spec == 'x' || spec == 'X' || spec == 'p') base = 16;
                char base_hex = spec == 'X' ? 'A' : 'a';
                
                char buf[30];
                int bptr = 0;
                if (data == 0) {
                    buf[bptr++] = '0';
                } else {
                    for (; data > 0; data /= base) {
                        int digit = data % base;
                        char c = '0' + digit;
                        if (digit >= 10) c = base_hex + digit - 10;
                        buf[bptr++] = c;
                    }
                }
                if (sign && left_pad == '0') {
                    addchar(sign);
                }
                for (int i = bptr + !!sign; i < width; i++) {
                    addchar(left_pad);
                }
                if (sign && left_pad != '0') {
                    addchar(sign);
                }
                for (bptr--; bptr >= 0; bptr--) {
                    addchar(buf[bptr]);
                }
            } break;
        }
    }
