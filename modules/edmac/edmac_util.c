#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <edmac.h>

#define STR_APPEND(orig,fmt,...) ({ int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); });

static char * edmac_format_size_3(
    int x, int off
)
{
    static char buf[32];
    snprintf(buf, sizeof(buf),
        off ? "%d, skip %d" : x ? "%d" : "",
        x, off
    );
    return buf;
}

static char * edmac_format_size_2(
    int y, int x,
    int off1, int off2
)
{
    static char buf[64]; buf[0] = 0;
    if (off1 == off2)
    {
        char * inner = edmac_format_size_3(x, off1);
        snprintf(buf, sizeof(buf),
            y == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, y+1
        );
    }
    else
    {
        /* y may be executed never, once or many times */
        if (y)
        {
            char * inner1 = edmac_format_size_3(x, off1);
            snprintf(buf, sizeof(buf),
                y == 0 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, y
            );
        }
        
        /* y is executed once */
        char * inner2 = edmac_format_size_3(x, off2);
        STR_APPEND(buf, "%s%s", buf[0] && inner2[0] ? ", " : "", inner2);
    }
    return buf;
}

static char * edmac_format_size_1(
    int y, int xn, int xa, int xb,
    int off1a, int off1b, int off2, int off3
)
{
    static char buf[128]; buf[0] = 0;
    if (xa == xb && off1a == off1b && off2 == off3)
    {
        char * inner = edmac_format_size_2(y, xa, off1a, off2);
        snprintf(buf, sizeof(buf),
            xn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, xn+1
        );
    }
    else
    {
        /* xa may be executed never, once or many times */
        if (xn)
        {
            char * inner1 = edmac_format_size_2(y, xa, off1a, off2);
            snprintf(buf, sizeof(buf),
                xn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, xn
            );
        }
        
        /* xb is executed once */
        char * inner2 = edmac_format_size_2(y, xb, off1b, off3);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

static char * edmac_format_size_0(
    int yn, int ya, int yb, int xn, int xa, int xb,
    int off1a, int off1b, int off2a, int off2b, int off3
)
{
#if 0
    const char * names[] = { "yn", "ya", "yb", "xn", "xa", "xb", "off1a", "off1b", "off2a", "off2b", "off3" };
    int values[] = { yn, ya, yb, xn, xa, xb, off1a, off1b, off2a, off2b, off3 };
    int len = 0;
    for (int i = 0; i < COUNT(values); i++)
        if (values[i])
            len += printf("%s=%d, ", names[i], values[i]);
    printf("\b\b: ");
    for (int i = 0; i < 45 - len; i++)
        printf(" ");
    if (len > 45)
        printf("\n  ");
#endif

    static char buf[256]; buf[0] = 0;
    
    if (ya == yb && off2a == off2b)
    {
        char * inner = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
        snprintf(buf, sizeof(buf),
            yn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, yn+1
        );
    }
    else
    {
        /* ya may be executed never, once or many times */
        if (yn)
        {
            char * inner1 = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
            snprintf(buf, sizeof(buf),
                yn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, yn
            );
        }
        
        /* yb is executed once */
        /* setting the last offset to off1b usually simplifies the formula */
        char * inner2 = edmac_format_size_1(yb, xn, xa, xb, off1a, off1b, off2b, off3 ? off3 : off1b);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

char * edmac_format_size(struct edmac_info * info)
{
    return edmac_format_size_0(
        info->yn, info->ya, info->yb,
        info->xn, info->xa, info->xb,
        info->off1a, info->off1b, info->off2a, info->off2b, info->off3
    );
}
