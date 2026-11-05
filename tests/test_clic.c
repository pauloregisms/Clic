/* Include the implementation to test private framing and discovery helpers too.
 * Each case runs in its own process, with a timeout, and uses local sockets. */
#include "../Clic.c"
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

typedef struct { Server *server; Client *sender; Client *receiver; } Connection;
static Connection connection(void) {
    Connection c;
    c.server = Server_create(0); assert(c.server);
    c.sender = Client_connect("127.0.0.1", c.server->port); assert(c.sender);
    c.receiver = Server_accept(c.server); assert(c.receiver);
    return c;
}
static void closeConnection(Connection c) { Client_free(c.sender); Server_close(c.server); }
static Client *pairClient(int *peer) {
    int fds[2]; assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    struct sockaddr_in address = { .sin_family = AF_INET };
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    *peer = fds[1];
    Client *c = Client_createFromSocket(fds[0], &address); assert(c); return c;
}
static void utf8_width(void) {
    assert(Clic_textWidth("Ação") == 4);
    assert(Clic_textWidth("界界") == 4);
    assert(Clic_textWidth("e\u0301") == 1);
    assert(Clic_textWidth(NULL) == -1);
    assert(Clic_textWidth("\xff") == -1);
    assert(Clic_textWidth("\033[31m") == -1);
}
static void unicode_prefix(void) {
    int width; const char *s = "界e\u0301a";
    size_t bytes = Text_prefix(s, 3, &width);
    assert(width == 3); assert(bytes == strlen("界e\u0301"));
}
static void table_utf8(void) {
    Table *t = Table_create("%-s %d %6.2f"); assert(t);
    Table_addRow(t, "Ação 界", 2, 8.5);
    assert(t->nRows == 1); assert(t->nCols == 3);
    assert(!strcmp(t->firstRow->cell[0].content, "Ação 界"));
    assert(!strcmp(t->firstRow->cell[1].content, "2"));
    t->width = 40; Table_print(t); Table_free(t);
}
static void table_fixed_width(void) {
    Table *t = Table_create("%-20s %-15s %-5s"); assert(t);
    Table_addRow(t,"Laboratório","127.0.0.1","5000");
    t->width = 50; Table_print(t); Table_free(t);
}
static void table_narrow(void) {
    Table *t = Table_create("%-20s %-15s %-5s"); assert(t);
    Table_addRow(t,"界界","127.0.0.1","5000");
    t->width = 3; Table_print(t); Table_free(t);
}
static int freed;
static void freeData(void *data) { freed++; free(data); }
static void table_userdata(void) {
    Table *t=Table_create("%s"); assert(t);
    assert(t->firstSelectableIndex==1 && !t->userData && !t->userDataFree);
    t->userData=malloc(10); assert(t->userData); t->userDataFree=freeData;
    Table_free(t); assert(freed==1);
}
static void screen_box_api(void) { Clic_printScreenBox(); }
static void invalid_inputs(void) {
    assert(!Server_create(-1)); assert(errno==EINVAL);
    assert(!Server_create(65536)); assert(!Client_connect(NULL,5000));
    assert(!Client_connect("127.0.0.1",0));
    char b[Client_MESSAGE_SIZE];
    assert(Client_receive(NULL,b)==-1); assert(Client_send(NULL,"a")==-1);
    assert(!Server_receive(NULL,b));
    assert(!Network_connectDiscovered(NULL,2));
    Client_free(NULL); Server_close(NULL);
}
static void client_no_data(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    assert(Client_receiveNonBlocking(c,b)==Client_NO_DATA);
    assert(Client_receiveLineNonBlocking(c,b)==Client_NO_DATA);
    assert(Client_isConnected(c)); Client_free(c); close(peer);
}
static void client_fragmented(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    assert(write(peer,"hel",3)==3);
    assert(Client_receiveLineNonBlocking(c,b)==Client_NO_DATA);
    assert(write(peer,"lo\n",3)==3);
    assert(Client_receiveLine(c,b)==5); assert(!strcmp(b,"hello"));
    Client_free(c); close(peer);
}
static void client_multiple_lines(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    assert(write(peer,"a\nb\n",4)==4);
    assert(Client_receiveLine(c,b)==1 && !strcmp(b,"a"));
    assert(Client_receiveLine(c,b)==1 && !strcmp(b,"b"));
    Client_free(c); close(peer);
}
static void line_endings(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    assert(write(peer,"\n\r\nx\r\n",7)==7);
    assert(Client_receiveLine(c,b)==0 && b[0]==0);
    assert(Client_receiveLine(c,b)==0 && b[0]==0);
    assert(Client_receiveLine(c,b)==1 && !strcmp(b,"x"));
    Client_free(c); close(peer);
}
static void send_line(void) {
    int peer; Client *c=pairClient(&peer); char b[16]={0};
    assert(Client_sendLine(c,"abc")==4);
    assert(Client_sendLine(c,"d\n")==2);
    assert(Client_sendLine(c,"")==1);
    assert(read(peer,b,7)==7); assert(!strcmp(b,"abc\nd\n\n"));
    Client_free(c); close(peer);
}
static void line_truncation(void) {
    int peer; Client *c=pairClient(&peer); char data[306], b[Client_MESSAGE_SIZE];
    memset(data,'x',300); memcpy(data+300,"\nend\n",5);
    assert(write(peer,data,305)==305);
    assert(Client_receiveLine(c,b)==255); assert(strlen(b)==255);
    assert(Client_receiveLine(c,b)==3 && !strcmp(b,"end"));
    Client_free(c); close(peer);
}
static void line_overflow(void) {
    int peer; Client *c=pairClient(&peer); char data[4500], b[Client_MESSAGE_SIZE];
    memset(data,'x',sizeof(data)); assert(write(peer,data,sizeof(data))==(ssize_t)sizeof(data));
    assert(Client_receiveLine(c,b)==-1); assert(errno==EMSGSIZE);
    assert(!Client_isConnected(c)); Client_free(c); close(peer);
}
static void raw_buffer_handoff(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    assert(write(peer,"a\nraw",5)==5);
    assert(Client_receiveLine(c,b)==1);
    assert(Client_receive(c,b)==3 && !strcmp(b,"raw"));
    Client_free(c); close(peer);
}
static void remote_eof(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE]; close(peer);
    assert(Client_receive(c,b)==-1); assert(!Client_isConnected(c));
    assert(c->socket==-1); Client_disconnect(c); Client_free(c);
}
static void sigpipe_handler(int signo) { (void)signo; _exit(99); }
static void sigpipe_preserved(void) {
    assert(signal(SIGPIPE,sigpipe_handler)!=SIG_ERR);
    Connection tcp=connection(); closeConnection(tcp);
    struct sigaction action; assert(sigaction(SIGPIPE,NULL,&action)==0);
    assert(action.sa_handler==sigpipe_handler);
    int peer; Client *c=pairClient(&peer); close(peer);
    assert(Client_send(c,"x")==-1);
    assert(sigaction(SIGPIPE,NULL,&action)==0 && action.sa_handler==sigpipe_handler);
    Client_free(c);
}
static void high_descriptor(void) {
    int peer; Client *c=pairClient(&peer); char b[Client_MESSAGE_SIZE];
    int high=fcntl(c->socket,F_DUPFD,1024); assert(high>=1024);
    close(c->socket); c->socket=high;
    assert(Client_receiveNonBlocking(c,b)==Client_NO_DATA);
    assert(write(peer,"ok\n",3)==3);
    assert(Client_receiveLine(c,b)==2 && !strcmp(b,"ok"));
    Client_free(c); close(peer);
}
static void server_multiple_lines(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    assert(Client_send(c.sender,"one\ntwo\n")==8);
    assert(Server_receiveLine(c.server,b)==c.receiver && !strcmp(b,"one"));
    assert(Server_receiveLineNonBlocking(c.server,b)==c.receiver && !strcmp(b,"two"));
    assert(!Server_receiveLineNonBlocking(c.server,b)); closeConnection(c);
}
static void server_multiple_blocking(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    assert(Client_send(c.sender,"one\ntwo\n")==8);
    assert(Server_receiveLine(c.server,b)==c.receiver && !strcmp(b,"one"));
    assert(Server_receiveLine(c.server,b)==c.receiver && !strcmp(b,"two"));
    closeConnection(c);
}
static int messages, connects, disconnects;
static void onMessage(Client *client,char *message) { assert(client && message); messages++; }
static void onConnect(Client *client) { assert(client); connects++; }
static void onDisconnect(Client *client) { assert(client); disconnects++; }
static void server_empty_line(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    Server_onMessage(c.server,onMessage);
    assert(Client_sendLine(c.sender,"")==1);
    assert(Server_receiveLine(c.server,b)==c.receiver && b[0]==0);
    assert(messages==1); closeConnection(c);
}
static void server_fragmented(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    assert(Client_send(c.sender,"part")==4);
    assert(!Server_receiveLineNonBlocking(c.server,b));
    assert(Client_send(c.sender,"ial\n")==4);
    assert(Server_receiveLine(c.server,b)==c.receiver && !strcmp(b,"partial"));
    closeConnection(c);
}
static void callbacks_lifecycle(void) {
    Server *s=Server_create(0); assert(s);
    Server_onConnect(s,onConnect); Server_onDisconnect(s,onDisconnect);
    Client *out=Client_connect("127.0.0.1",s->port); assert(out);
    assert(Server_acceptNonBlocking(s)); assert(connects==1);
    assert(Server_clientCount(s)==1); Client_free(out);
    char b[Client_MESSAGE_SIZE]; assert(!Server_receiveNonBlocking(s,b));
    assert(disconnects==1 && Server_clientCount(s)==0); Server_close(s);
}
static void auto_accept(void) {
    Server *s=Server_create(0); assert(s); char b[Client_MESSAGE_SIZE];
    Client *out=Client_connect("127.0.0.1",s->port); assert(out);
    assert(Client_sendLine(out,"hello")==6);
    assert(Server_receiveLine(s,b)); assert(!strcmp(b,"hello"));
    Client_free(out); Server_close(s);
}
static void broadcasts(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    Client *other=Client_connect("127.0.0.1",c.server->port); assert(other);
    assert(Server_accept(c.server));
    Server_sendOthers(c.server,c.receiver,"other\n");
    assert(Client_receiveLine(other,b)==5 && !strcmp(b,"other"));
    assert(Client_receiveLineNonBlocking(c.sender,b)==Client_NO_DATA);
    Server_sendAll(c.server,"all\n");
    assert(Client_receiveLine(other,b)==3 && !strcmp(b,"all"));
    assert(Client_receiveLine(c.sender,b)==3 && !strcmp(b,"all"));
    Client_free(other); closeConnection(c);
}
static void names(void) {
    Connection c=connection(); char longName[200]; memset(longName,'x',199); longName[199]=0;
    Client_setName(c.sender,longName); assert(strlen(Client_getName(c.sender))==63);
    assert(!strcmp(Client_getIP(c.sender),"127.0.0.1"));
    assert(Client_getPort(c.sender)==c.server->port);
    Server_setName(c.server,"room|x\n"); assert(!strcmp(Server_getName(c.server),"room x "));
    closeConnection(c);
}
static void discovery_dedup(void) {
    NetworkDiscoveryData d={0};
    Network_addDiscoveredServer(&d,"same","127.0.0.1",5000);
    Network_addDiscoveredServer(&d,"same","127.0.0.2",5000);
    Network_addDiscoveredServer(&d,"renamed","127.0.0.1",5000);
    Network_addDiscoveredServer(&d,"same","127.0.0.1",5001);
    assert(d.count==3);
    Network_addDiscoveredServer(&d,"bad","127.0.0.1",0); assert(d.count==3);
}
static void discovery_table(void) {
    Table *t=Network_createDiscoveryTable(); assert(t);
    assert(!strcmp(t->firstRow->cell[2].content,"Porta"));
    assert(Network_discoveredCount(t)==0 && t->firstSelectableIndex==2);
    assert(Table_select(t)==-1);
    t->width=50; Table_print(t); Table_free(t);
}
static void discovery_udp(void) {
    Server *s=Server_create(0); assert(s); Server_setName(s,"local");
    assert(Server_enableDiscovery(s)); assert(Server_enableDiscovery(s));
    int fd=socket(AF_INET,SOCK_DGRAM,0); assert(fd>=0);
    struct sockaddr_in dst={.sin_family=AF_INET,.sin_port=htons(Network_DISCOVERY_PORT)};
    dst.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    char query[]=Clic_DISCOVERY_QUERY;
    assert(sendto(fd,query,strlen(query),0,(struct sockaddr*)&dst,sizeof(dst))==(ssize_t)strlen(query));
    struct pollfd ready={.fd=s->discoverySocket,.events=POLLIN};
    assert(Network_poll(&ready,1,1000)==1); Server_update(s);
    ready.fd=fd; assert(Network_poll(&ready,1,1000)==1);
    char response[128]={0},expected[128]; assert(recv(fd,response,sizeof(response)-1,0)>0);
    snprintf(expected,sizeof(expected),"%s|local|%d",Clic_DISCOVERY_RESPONSE,s->port);
    assert(!strcmp(response,expected)); close(fd);
    Server_disableDiscovery(s); assert(!s->discoveryEnabled && s->discoverySocket==-1);
    Server_close(s);
}
static void discovered_connection(void) {
    Server *s=Server_create(0); assert(s);
    Table *t=Network_createDiscoveryTable(); assert(t);
    Network_addDiscoveredServer(t->userData,"local","127.0.0.1",s->port);
    assert(Network_discoveredCount(t)==1);
    assert(!Network_connectDiscovered(t,1)); assert(!Network_connectDiscovered(t,3));
    Client *out=Network_connectDiscovered(t,2); assert(out); assert(Server_accept(s));
    Client_free(out); Table_free(t); Server_close(s);
}
static void clients_limit(void) {
    Server *s=Server_create(0); assert(s); Client *out[Server_MAX_CLIENTS+1];
    for(int i=0;i<=Server_MAX_CLIENTS;i++) {
        out[i]=Client_connect("127.0.0.1",s->port); assert(out[i]);
        Client *accepted=Server_accept(s);
        if(i<Server_MAX_CLIENTS) assert(accepted);
        else { assert(!accepted); assert(errno==ENOSPC); }
    }
    assert(Server_clientCount(s)==Server_MAX_CLIENTS);
    for(int i=0;i<=Server_MAX_CLIENTS;i++) Client_free(out[i]);
    Server_close(s);
}
static FILE *capture_begin(int *saved) {
    fflush(stdout);
    *saved=dup(STDOUT_FILENO); assert(*saved>=0);
    FILE *file=tmpfile(); assert(file);
    assert(dup2(fileno(file),STDOUT_FILENO)>=0); return file;
}
static char *capture_end(FILE *file,int saved) {
    fflush(stdout); long length=ftell(file); assert(length>=0);
    assert(dup2(saved,STDOUT_FILENO)>=0); close(saved);
    rewind(file); char *output=calloc((size_t)length+1,1); assert(output);
    assert(fread(output,1,(size_t)length,file)==(size_t)length); fclose(file); return output;
}
static void unicode_rendering(void) {
    int saved; FILE *file=capture_begin(&saved);
    Text_printCell("界e\u0301abc",4,true);
    char *output=capture_end(file,saved);
    assert(!strcmp(output,"界e\u0301…")); free(output);
}
static void table_rendering(void) {
    Table *t=Table_create("%-14s %-s %6.2f"); assert(t); t->width=58;
    Table_addRow(t,"João","Programação",8.5);
    int saved; FILE *file=capture_begin(&saved); Table_print(t);
    char *output=capture_end(file,saved);
    assert(strstr(output,"João") && strstr(output,"Programação") && strstr(output,"8.50"));
    /* Every segment between ANSI cursor escapes is valid UTF-8. */
    for(char *p=output;*p;) {
        if(*p=='\033') { p++; if(*p=='[') { p++; while(*p && !isalpha((unsigned char)*p)) p++; if(*p)p++; } else if(*p)p++; }
        else { char *end=strchr(p,'\033'); char save=0; if(end){save=*end;*end=0;}
            assert(Clic_textWidth(p)>=0); if(end){*end=save;p=end;}else break; }
    }
    free(output); Table_free(t);
}
static Server *callbackServer;
static void reentrantDisconnect(Client *client) {
    assert(client); assert(Server_clientCount(callbackServer)==0); disconnects++;
}
static void callback_query_on_disconnect(void) {
    Connection c=connection(); callbackServer=c.server;
    Server_onDisconnect(c.server,reentrantDisconnect);
    Client_disconnect(c.receiver); assert(Server_clientCount(c.server)==0);
    assert(disconnects==1); closeConnection(c);
}
static void discovery_invalid_name(void) {
    NetworkDiscoveryData data={0}; char name[100]; memset(name,'x',99);name[99]=0;
    Network_addDiscoveredServer(&data,name,"127.0.0.1",5000);
    Network_addDiscoveredServer(&data,"bad\033[0m","127.0.0.1",5000);
    assert(data.count==0);
}
static void discovery_socket_failure(void) {
    struct rlimit limit={32,32}; assert(setrlimit(RLIMIT_NOFILE,&limit)==0);
    int fds[32],count=0;
    while(count<32) {int fd=open("/dev/null",O_RDONLY);if(fd<0)break;fds[count++]=fd;}
    assert(!Network_discoverServers()); assert(errno==EMFILE);
    while(count)close(fds[--count]);
}
static void server_high_descriptor(void) {
    Connection c=connection(); char b[Client_MESSAGE_SIZE];
    int high=fcntl(c.receiver->socket,F_DUPFD,1024); assert(high>=1024);
    close(c.receiver->socket); c.receiver->socket=high;
    assert(Client_sendLine(c.sender,"high")==5);
    assert(Server_receiveLine(c.server,b)==c.receiver && !strcmp(b,"high"));
    closeConnection(c);
}
#define CASE(fn) {#fn,fn}
int main(void) {
    assert(setlocale(LC_CTYPE,"C.UTF-8"));
    struct { const char *name; void (*run)(void); } cases[]={
        CASE(utf8_width),CASE(unicode_prefix),CASE(table_utf8),CASE(table_fixed_width),
        CASE(table_narrow),CASE(table_userdata),CASE(screen_box_api),CASE(invalid_inputs),
        CASE(client_no_data),CASE(client_fragmented),CASE(client_multiple_lines),CASE(line_endings),
        CASE(send_line),CASE(line_truncation),CASE(line_overflow),CASE(raw_buffer_handoff),
        CASE(remote_eof),CASE(sigpipe_preserved),CASE(high_descriptor),CASE(server_multiple_lines),
        CASE(server_multiple_blocking),CASE(server_empty_line),CASE(server_fragmented),
        CASE(callbacks_lifecycle),CASE(auto_accept),CASE(broadcasts),CASE(names),
        CASE(discovery_dedup),CASE(discovery_table),CASE(discovery_udp),
        CASE(discovered_connection),CASE(clients_limit),CASE(unicode_rendering),CASE(table_rendering),
        CASE(callback_query_on_disconnect),CASE(discovery_invalid_name),
        CASE(discovery_socket_failure),CASE(server_high_descriptor)
    };
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(*cases);i++) {
        fflush(NULL); pid_t pid=fork(); assert(pid>=0);
        if(!pid) {
            alarm(5);
            struct rlimit core={0,0}; setrlimit(RLIMIT_CORE,&core);
            assert(freopen("/dev/null","w",stdout));
            cases[i].run(); exit(0);
        }
        int status; assert(waitpid(pid,&status,0)==pid);
        bool ok=WIFEXITED(status) && WEXITSTATUS(status)==0;
        printf("%s %s\n",ok?"PASS":"FAIL",cases[i].name); if(!ok) failures++;
    }
    printf("%zu tests, %d failures\n",sizeof(cases)/sizeof(*cases),failures);
    return failures?1:0;
}
