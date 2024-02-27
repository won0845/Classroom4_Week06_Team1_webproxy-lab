#include "csapp.h"

int main(void)
{
  char *buf, *pair;
  char content[MAXLINE];
  int n1 = 0, n2 = 0;

  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    pair = strtok(buf, "&");
    while (pair != NULL)
    {
      sscanf(pair, "num1=%d", &n1);
      sscanf(pair, "num2=%d", &n2);
      pair = strtok(NULL, "&");
    }
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
          content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}