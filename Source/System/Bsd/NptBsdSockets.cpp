/*****************************************************************
|
|   Neptune - Sockets :: BSD/Winsock Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if defined(WIN32) || defined(UNDER_CE)

// Win32 includes
#define __WIN32__
#endif
#if defined(__WIN32__)
#define STRICT
#define NPT_WIN32_USE_WINSOCK2
#ifdef NPT_WIN32_USE_WINSOCK2
/* it is important to include this in this order, because winsock.h and ws2tcpip.h */
/* have different definitions for the same preprocessor symbols, such as IP_ADD_MEMBERSHIP */
#include <winsock2.h>
#include <ws2tcpip.h> 
#else
#include <winsock.h>
#endif
#include <windows.h>

// XBox
#elif defined(_XBOX)
#include <xtl.h>
#include <winsockx.h>

#elif defined(__TCS__)

// Trimedia includes
#include <sockets.h>

#elif defined(__PSP__)

// PSP includes
#include <psptypes.h>
#include <kernel.h>
#include <pspnet.h>
#include <pspnet_error.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <pspnet_apctl.h>
#include <pspnet_ap_dialog_dummy.h>
#include <errno.h>
#include <wlan.h>
#include <pspnet/sys/socket.h>
#include <pspnet/sys/select.h>
#include <pspnet/netinet/in.h>

#elif defined(__PPU__)

// PS3 includes
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <netex/net.h>
#include <netex/errno.h>

#else

// default includes
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#endif 


#include "NptConfig.h"
#include "NptTypes.h"
#include "NptStreams.h"
#include "NptThreads.h"
#include "NptSockets.h"
#include "NptUtils.h"
#include "NptConstants.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_TCP_SERVER_SOCKET_DEFAULT_LISTEN_COUNT = 20;

/*----------------------------------------------------------------------
|   WinSock adaptation layer
+---------------------------------------------------------------------*/
#if defined(__WIN32__) || defined(_XBOX)
#if defined(_XBOX)
#include "NptXboxNetwork.h"
#define SO_ERROR    0x1007          /* unsupported */
#else
#include "NptWin32Network.h"
#endif
// force a reference to the initializer so that the linker does not optimize it out
static NPT_WinsockSystem& WinsockInitializer = NPT_WinsockSystem::Initializer; 

#if defined(SetPort)
#undef SetPort
#endif

#define EWOULDBLOCK  WSAEWOULDBLOCK
#define EINPROGRESS  WSAEINPROGRESS
#define ECONNREFUSED WSAECONNREFUSED
#define ECONNABORTED WSAECONNABORTED
#define ECONNRESET   WSAECONNRESET
#define ETIMEDOUT    WSAETIMEDOUT
#define ENETRESET    WSAENETRESET
#define EADDRINUSE   WSAEADDRINUSE
#define ENETDOWN     WSAENETDOWN
#define ENETUNREACH  WSAENETUNREACH
#if !defined(EAGAIN)
#define EAGAIN       WSAEWOULDBLOCK 
#endif

typedef int         ssize_t;
typedef int         socklen_t;
typedef char*       SocketBuffer;
typedef const char* SocketConstBuffer;
typedef char*       SocketOption;
typedef SOCKET      SocketFd;

#define GetSocketError()                 WSAGetLastError()
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s) == INVALID_SOCKET)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e) == SOCKET_ERROR)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e) == SOCKET_ERROR)

/*----------------------------------------------------------------------
|   Trimedia adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__TCS__)  // trimedia PSOS w/ Target TCP
typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define GetSocketError()                 errno
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s)  < 0)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e)  < 0)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e)  < 0)

/*----------------------------------------------------------------------
|   PSP adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__PSP__)
typedef SceNetInetSocklen_t socklen_t;
#define timeval SceNetInetTimeval
#define inet_addr sceNetInetInetAddr
#define select sceNetInetSelect
#define socket sceNetInetSocket
#define connect sceNetInetConnect
#define bind sceNetInetBind
#define accept sceNetInetAccept
#define getpeername sceNetInetGetpeername
#define getsockopt sceNetInetGetsockopt
#define setsockopt sceNetInetSetsockopt
#define listen sceNetInetListen
#define getsockname sceNetInetGetsockname
#define sockaddr SceNetInetSockaddr
#define sockaddr_in SceNetInetSockaddrIn
#define in_addr SceNetInetInAddr
#define send  sceNetInetSend
#define sendto sceNetInetSendto
#define recv  sceNetInetRecv
#define recvfrom sceNetInetRecvfrom
#define closesocket sceNetInetClose
#define htonl sceNetHtonl
#define htons sceNetHtons
#define ntohl sceNetNtohl
#define ntohs sceNetNtohs
#define SOL_SOCKET SCE_NET_INET_SOL_SOCKET
#define AF_INET SCE_NET_INET_AF_INET
#define SOCK_STREAM SCE_NET_INET_SOCK_STREAM
#define SOCK_DGRAM SCE_NET_INET_SOCK_DGRAM
#define SO_BROADCAST SCE_NET_INET_SO_BROADCAST
#define SO_ERROR SCE_NET_INET_SO_ERROR
#define IPPROTO_IP SCE_NET_INET_IPPROTO_IP
#define IP_ADD_MEMBERSHIP SCE_NET_INET_IP_ADD_MEMBERSHIP
#define IP_MULTICAST_IF SCE_NET_INET_IP_MULTICAST_IF
#define IP_MULTICAST_TTL SCE_NET_INET_IP_MULTICAST_TTL
#define SO_REUSEADDR SCE_NET_INET_SO_REUSEADDR
#define INADDR_ANY SCE_NET_INET_INADDR_ANY
#define ip_mreq SceNetInetIpMreq
#ifdef fd_set
#undef fd_set
#endif
#define fd_set SceNetInetFdSet
#ifdef FD_ZERO
#undef FD_ZERO
#endif
#define FD_ZERO SceNetInetFD_ZERO
#ifdef FD_SET
#undef FD_SET
#endif
#define FD_SET SceNetInetFD_SET
#ifdef FD_CLR
#undef FD_CLR
#endif
#define FD_CLR SceNetInetFD_CLR
#ifdef FD_ISSET
#undef FD_ISSET
#endif
#define FD_ISSET SceNetInetFD_ISSET

#define RESOLVER_TIMEOUT (5 * 1000 * 1000)
#define RESOLVER_RETRY    5

typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define GetSocketError()                 sceNetInetGetErrno()
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s) < 0)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e) < 0)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e) < 0)

/*----------------------------------------------------------------------
|   PS3 adaptation layer
+---------------------------------------------------------------------*/
#elif defined(__PPU__)
#undef EWOULDBLOCK    
#undef ECONNREFUSED  
#undef ECONNABORTED  
#undef ECONNRESET    
#undef ETIMEDOUT     
#undef ENETRESET     
#undef EADDRINUSE    
#undef ENETDOWN      
#undef ENETUNREACH   
#undef EAGAIN        
#undef EINTR     
#undef EINPROGRESS

#define EWOULDBLOCK   SYS_NET_EWOULDBLOCK 
#define ECONNREFUSED  SYS_NET_ECONNREFUSED
#define ECONNABORTED  SYS_NET_ECONNABORTED
#define ECONNRESET    SYS_NET_ECONNRESET
#define ETIMEDOUT     SYS_NET_ETIMEDOUT
#define ENETRESET     SYS_NET_ENETRESET
#define EADDRINUSE    SYS_NET_EADDRINUSE
#define ENETDOWN      SYS_NET_ENETDOWN
#define ENETUNREACH   SYS_NET_ENETUNREACH
#define EAGAIN        SYS_NET_EAGAIN
#define EINTR         SYS_NET_EINTR
#define EINPROGRESS   SYS_NET_EINPROGRESS

typedef void*        SocketBuffer;
typedef const void*  SocketConstBuffer;
typedef void*        SocketOption;
typedef int          SocketFd;

#define closesocket  socketclose
#define select       socketselect

#define GetSocketError()                 sys_net_errno
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s) < 0)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e) < 0)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e) < 0)

// network initializer 
static struct NPT_Ps3NetworkInitializer {
    NPT_Ps3NetworkInitializer() {
        sys_net_initialize_network();
    }
    ~NPT_Ps3NetworkInitializer() {
        sys_net_finalize_network();
    }
} Ps3NetworkInitializer;

/*----------------------------------------------------------------------
|   Default adaptation layer
+---------------------------------------------------------------------*/
#else  
typedef void*       SocketBuffer;
typedef const void* SocketConstBuffer;
typedef void*       SocketOption;
typedef int         SocketFd;

#define closesocket  close
#define ioctlsocket  ioctl

#define GetSocketError()                 errno
#define NPT_BSD_SOCKET_IS_INVALID(_s)    ((_s)  < 0)
#define NPT_BSD_SOCKET_CALL_FAILED(_e)   ((_e)  < 0)
#define NPT_BSD_SOCKET_SELECT_FAILED(_e) ((_e)  < 0)

#endif

/*----------------------------------------------------------------------
|   SocketAddressToInetAddress
+---------------------------------------------------------------------*/
static void
SocketAddressToInetAddress(const NPT_SocketAddress& socket_address, 
                           struct sockaddr_in*      inet_address)
{
    // initialize the structure
    for (int i=0; i<8; i++) inet_address->sin_zero[i]=0;

    // setup the structure
    inet_address->sin_family = AF_INET;
    inet_address->sin_port = htons(socket_address.GetPort());
    inet_address->sin_addr.s_addr = htonl(socket_address.GetIpAddress().AsLong());
}

/*----------------------------------------------------------------------
|   InetAddressToSocketAddress
+---------------------------------------------------------------------*/
static void
InetAddressToSocketAddress(const struct sockaddr_in* inet_address,
                           NPT_SocketAddress&        socket_address)
{
    // read the fields
    socket_address.SetPort(ntohs(inet_address->sin_port));
    socket_address.SetIpAddress(NPT_IpAddress(ntohl(inet_address->sin_addr.s_addr)));
}

/*----------------------------------------------------------------------
|   MapErrorCode
+---------------------------------------------------------------------*/
static NPT_Result 
MapErrorCode(int error)
{
    switch (error) {
        case ECONNRESET:
        case ENETRESET:
            return NPT_ERROR_CONNECTION_RESET;

        case ECONNABORTED:
            return NPT_ERROR_CONNECTION_ABORTED;

        case ECONNREFUSED:
            return NPT_ERROR_CONNECTION_REFUSED;

        case ETIMEDOUT:
            return NPT_ERROR_TIMEOUT;

        case EADDRINUSE:
            return NPT_ERROR_ADDRESS_IN_USE;

        case ENETDOWN:
            return NPT_ERROR_NETWORK_DOWN;

        case ENETUNREACH:
            return NPT_ERROR_NETWORK_UNREACHABLE;

        case EINPROGRESS:
        case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
            return NPT_ERROR_WOULD_BLOCK;

        default:
            return NPT_FAILURE;
    }
}

#if defined(_XBOX)

struct hostent {
    char    * h_name;           /* official name of host */
    char    * * h_aliases;      /* alias list */
    short   h_addrtype;         /* host address type */
    short   h_length;           /* length of address */
    char    * * h_addr_list;    /* list of addresses */
#define h_addr  h_addr_list[0]  /* address, for backward compat */
};

typedef struct {
    struct hostent server;
    char name[128];
    char addr[16];
    char* addr_list[4];
} HostEnt;

/*----------------------------------------------------------------------
|   gethostbyname
+---------------------------------------------------------------------*/
static struct hostent* 
gethostbyname(const char* name)
{
    struct hostent* host = NULL;
    HostEnt*        host_entry = new HostEnt;
    WSAEVENT        hEvent = WSACreateEvent();
    XNDNS*          pDns = NULL;

    INT err = XNetDnsLookup(name, hEvent, &pDns);
    WaitForSingleObject(hEvent, INFINITE);
    if (pDns) {
        if (pDns->iStatus == 0) {
            strcpy(host_entry->name, name);
            host_entry->addr_list[0] = host_entry->addr;
            memcpy(host_entry->addr, &(pDns->aina[0].s_addr), 4);
            host_entry->server.h_name = host_entry->name;
            host_entry->server.h_aliases = 0;
            host_entry->server.h_addrtype = AF_INET;
            host_entry->server.h_length = 4;
            host_entry->server.h_addr_list = new char*[4];

            host_entry->server.h_addr_list[0] = host_entry->addr_list[0];
            host_entry->server.h_addr_list[1] = 0;

            host = (struct hostent*)host_entry;
        }
        XNetDnsRelease(pDns);
    }
    WSACloseEvent(hEvent);
    return host;
};

#endif // _XBOX

/*----------------------------------------------------------------------
|   NPT_IpAddress::ResolveName
+---------------------------------------------------------------------*/
NPT_Result
NPT_IpAddress::ResolveName(const char* name, NPT_Timeout)
{
    // check parameters
    if (name == NULL || name[0] == '\0') return NPT_ERROR_HOST_UNKNOWN;

    // handle numerical addrs
    NPT_IpAddress numerical_address;
    if (NPT_SUCCEEDED(numerical_address.Parse(name))) {
        /* the name is a numerical IP addr */
        return Set(numerical_address.AsLong());
    }

#if defined(__TCS__)
    Set(getHostByName(name));
#elif defined(__PSP__)
    int rid;
    char buf[1024];
    int buflen = sizeof(buf);

    int ret = sceNetResolverCreate(&rid, buf, buflen);
    if(ret < 0){
        return NPT_FAILURE;
    }
    ret = sceNetResolverStartNtoA(rid, name, &address->sin_addr,
        RESOLVER_TIMEOUT, RESOLVER_RETRY);
    if(ret < 0){
        return NPT_ERROR_HOST_UNKNOWN;
    }
    sceNetResolverDelete(rid);
#else
    // do a name lookup
    struct hostent *host_entry = gethostbyname(name);
    if (host_entry == NULL ||
        host_entry->h_addrtype != AF_INET) {
        return NPT_ERROR_HOST_UNKNOWN;
    }
    NPT_CopyMemory(m_Address, host_entry->h_addr, 4);

#if defined(_XBOX)
    delete host_entry;   
#endif

#endif
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketFd
+---------------------------------------------------------------------*/
class NPT_BsdSocketFd
{
public:
    // constructors and destructor
    NPT_BsdSocketFd(SocketFd fd) : 
      m_SocketFd(fd), 
      m_Blocking(true),
      m_ReadTimeout(NPT_TIMEOUT_INFINITE), 
      m_WriteTimeout(NPT_TIMEOUT_INFINITE) {}
    ~NPT_BsdSocketFd() {
        closesocket(m_SocketFd);
    }

    // methods
    NPT_Result SetBlockingMode(bool blocking);
    NPT_Result WaitUntilReadable();
    NPT_Result WaitUntilWriteable();

    // members
    SocketFd     m_SocketFd;
    bool         m_Blocking;
    NPT_Timeout  m_ReadTimeout;
    NPT_Timeout  m_WriteTimeout;
    NPT_Position m_Position;

private:
    // methods
    NPT_Result WaitForCondition(bool readable, bool writeable, NPT_Timeout timeout);
};

typedef NPT_Reference<NPT_BsdSocketFd> NPT_BsdSocketFdReference;

#if defined(__WIN32__) || defined(__TCS__) || defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::SetBlockingMode
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::SetBlockingMode(bool blocking)
{
    unsigned long args = blocking?0:1;
    if (ioctlsocket(m_SocketFd, FIONBIO, &args)) {
        return NPT_ERROR_SOCKET_CONTROL_FAILED;
    }
    m_Blocking = blocking;
    return NPT_SUCCESS;
}
#elif defined(__PSP__) || defined(__PPU__)
/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::SetBlockingMode
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::SetBlockingMode(bool blocking)
{
    int args = blocking?0:1;
    if (setsockopt(m_SocketFd, SOL_SOCKET, SO_NBIO, &args, sizeof(args))) {
        return NPT_ERROR_SOCKET_CONTROL_FAILED;
    }
    m_Blocking = blocking;
    return NPT_SUCCESS;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::SetBlockingMode
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::SetBlockingMode(bool blocking)
{
    int flags = fcntl(m_SocketFd, F_GETFL, 0);
    if (blocking) {
        flags ^= O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    if (fcntl(m_SocketFd, F_SETFL, flags)) {
        return NPT_ERROR_SOCKET_CONTROL_FAILED;
    }
    m_Blocking = blocking;
    return NPT_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::WaitUntilReadable
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::WaitUntilReadable()
{
    return WaitForCondition(true, false, m_ReadTimeout);
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::WaitUntilWriteable
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::WaitUntilWriteable()
{
    return WaitForCondition(false, true, m_WriteTimeout);
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketFd::WaitForCondition
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketFd::WaitForCondition(bool        wait_for_readable, 
                                  bool        wait_for_writeable, 
                                  NPT_Timeout timeout)
{
    // wait for incoming connection
    NPT_Result result = NPT_SUCCESS;
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;
    FD_ZERO(&read_set);
    if (wait_for_readable) FD_SET(m_SocketFd, &read_set);
    FD_ZERO(&write_set);
    if (wait_for_writeable) FD_SET(m_SocketFd, &write_set);
    FD_ZERO(&except_set);
    FD_SET(m_SocketFd, &except_set);

    struct timeval timeout_value;
    if (timeout != NPT_TIMEOUT_INFINITE) {
        timeout_value.tv_sec = timeout/1000;
        timeout_value.tv_usec = 1000*(timeout-1000*(timeout/1000));
    };
    
    int io_result = select((int)m_SocketFd+1, 
                            &read_set, &write_set, &except_set, 
                            timeout == NPT_TIMEOUT_INFINITE ? 
                            NULL : &timeout_value);

    if (io_result == 0) {
        if (timeout == 0) {
            // non-blocking call
            result = NPT_ERROR_WOULD_BLOCK;
        } else {
            // timeout
            result = NPT_ERROR_TIMEOUT;
        }
    } else if (NPT_BSD_SOCKET_SELECT_FAILED(io_result)) {
        result = MapErrorCode(GetSocketError());
    } else if (FD_ISSET(m_SocketFd, &except_set)) {
        result = MapErrorCode(GetSocketError());
    } else if (!(wait_for_readable  && FD_ISSET(m_SocketFd, &read_set)) && 
               !(wait_for_writeable && FD_ISSET(m_SocketFd, &write_set))) {
        // should not happen
        result = NPT_ERROR_INTERNAL;
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketStream
+---------------------------------------------------------------------*/
class NPT_BsdSocketStream
{
 public:
    // methods
    NPT_BsdSocketStream(NPT_BsdSocketFdReference& socket_fd) :
       m_SocketFdReference(socket_fd) {}

    // NPT_InputStream and NPT_OutputStream methods
    NPT_Result Seek(NPT_Position) { return NPT_ERROR_NOT_SUPPORTED; }
    NPT_Result Tell(NPT_Position& where) {
        where = 0;
        return NPT_SUCCESS;
    }

 protected:
    // destructor
     virtual ~NPT_BsdSocketStream() {}

    // members
    NPT_BsdSocketFdReference m_SocketFdReference;
};

/*----------------------------------------------------------------------
|   NPT_BsdSocketInputStream
+---------------------------------------------------------------------*/
class NPT_BsdSocketInputStream : public NPT_InputStream,
                                 private NPT_BsdSocketStream
{
public:
    // constructors and destructor
    NPT_BsdSocketInputStream(NPT_BsdSocketFdReference& socket_fd) :
      NPT_BsdSocketStream(socket_fd) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Position offset) { 
        return NPT_BsdSocketStream::Seek(offset); }
    NPT_Result Tell(NPT_Position& where) {
        return NPT_BsdSocketStream::Tell(where);
    }
    NPT_Result GetSize(NPT_Size& size);
    NPT_Result GetAvailable(NPT_Size& available);
};

/*----------------------------------------------------------------------
|   NPT_BsdSocketInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketInputStream::Read(void*     buffer, 
                               NPT_Size  bytes_to_read, 
                               NPT_Size* bytes_read)
{
    // if we're blocking, wait until the socket is readable
    if (m_SocketFdReference->m_Blocking && 
        m_SocketFdReference->m_ReadTimeout != NPT_TIMEOUT_INFINITE) {
        NPT_Result result = m_SocketFdReference->WaitUntilReadable();
        if (result != NPT_SUCCESS) return result;
    }

    // read from the socket
    ssize_t nb_read = recv(m_SocketFdReference->m_SocketFd, 
                           (SocketBuffer)buffer, 
                           bytes_to_read, 0);
    if (nb_read > 0) {
        if (bytes_read) *bytes_read = nb_read;
        m_SocketFdReference->m_Position += nb_read;
        return NPT_SUCCESS;
    } else {
        if (bytes_read) *bytes_read = 0;
        if (nb_read == 0) {
            return NPT_ERROR_EOS;
        } else {
            return MapErrorCode(GetSocketError());
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketInputStream::GetSize(NPT_Size& size)
{
    // generic socket streams have no size
    size = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

#if defined(__PPU__)
/*----------------------------------------------------------------------
|   NPT_BsdSocketInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketInputStream::GetAvailable(NPT_Size&)
{
    return NPT_ERROR_NOT_SUPPORTED;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdSocketInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketInputStream::GetAvailable(NPT_Size& available)
{
    unsigned long ready = 0;
    int io_result = ioctlsocket(m_SocketFdReference->m_SocketFd, FIONREAD, &ready); 
    if (NPT_BSD_SOCKET_CALL_FAILED(io_result)) {
        available = 0;
        return NPT_ERROR_SOCKET_CONTROL_FAILED;
    } else {
        available = ready;
        return NPT_SUCCESS;
    }
}
#endif

/*----------------------------------------------------------------------
|   NPT_BsdSocketOutputStream
+---------------------------------------------------------------------*/
class NPT_BsdSocketOutputStream : public NPT_OutputStream,
                                  private NPT_BsdSocketStream
{
public:
    // constructors and destructor
    NPT_BsdSocketOutputStream(NPT_BsdSocketFdReference& socket_fd) :
        NPT_BsdSocketStream(socket_fd) {}

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, 
                     NPT_Size    bytes_to_write, 
                     NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Position offset) { 
        return NPT_BsdSocketStream::Seek(offset); }
    NPT_Result Tell(NPT_Position& where) {
        return NPT_BsdSocketStream::Tell(where);
    }
    NPT_Result Flush();
};

/*----------------------------------------------------------------------
|   NPT_BsdSocketOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketOutputStream::Write(const void*  buffer, 
                                 NPT_Size     bytes_to_write, 
                                 NPT_Size*    bytes_written)
{
    // if we're blocking, wait until the socket is writeable
    if (m_SocketFdReference->m_Blocking && 
        m_SocketFdReference->m_WriteTimeout != NPT_TIMEOUT_INFINITE) {
        NPT_Result result = m_SocketFdReference->WaitUntilWriteable();
        if (result != NPT_SUCCESS) return result;
    }

    // write to the socket
    ssize_t nb_written = send(m_SocketFdReference->m_SocketFd, 
                             (SocketConstBuffer)buffer, 
                             bytes_to_write, 0);

    if (nb_written > 0) {
        if (bytes_written) *bytes_written = nb_written;
        m_SocketFdReference->m_Position += nb_written;
        return NPT_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        if (nb_written == 0) {
            return NPT_ERROR_CONNECTION_RESET;
        } else {
            return MapErrorCode(GetSocketError());
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocketOutputStream::Flush
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocketOutputStream::Flush()
{
    int       args = 0;
    socklen_t size = sizeof(args);

    // get the value of the nagle algorithm
    if (getsockopt(m_SocketFdReference->m_SocketFd, 
                  IPPROTO_TCP, 
                  TCP_NODELAY, 
                  (char*)&args, 
                  &size)) {
        return NPT_ERROR_GETSOCKOPT_FAILED;
    }

    // return now if nagle was already off
    if (args == 1) return NPT_SUCCESS;

    // disable the nagle algorithm
    args = 1;
    if (setsockopt(m_SocketFdReference->m_SocketFd, 
                   IPPROTO_TCP, 
                   TCP_NODELAY, 
                   (const char*)&args, 
                   sizeof(args))) {
        return NPT_ERROR_SETSOCKOPT_FAILED;
    }

    // send an empty buffer to flush
    char dummy;
    send(m_SocketFdReference->m_SocketFd, &dummy, 0, 0); 

    // restore the nagle algorithm to its original setting
    args = 0;
    if (setsockopt(m_SocketFdReference->m_SocketFd, 
                   IPPROTO_TCP, 
                   TCP_NODELAY, 
                   (const char*)&args, 
                   sizeof(args))) {
        return NPT_ERROR_SETSOCKOPT_FAILED;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket
+---------------------------------------------------------------------*/
class NPT_BsdSocket : public NPT_SocketInterface
{
 public:
    // constructors and destructor
             NPT_BsdSocket() {}
             NPT_BsdSocket(SocketFd fd, bool force_blocking=false);
    virtual ~NPT_BsdSocket();

    // methods
    NPT_Result RefreshInfo();

    // NPT_SocketInterface methods
    NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true);
    NPT_Result Connect(const NPT_SocketAddress& address, NPT_Timeout timeout);
    NPT_Result WaitForConnection(NPT_Timeout timeout);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);
    NPT_Result GetInfo(NPT_SocketInfo& info);
    NPT_Result SetBlockingMode(bool blocking);
    NPT_Result SetReadTimeout(NPT_Timeout timeout);
    NPT_Result SetWriteTimeout(NPT_Timeout timeout);

 protected:
    // members
    NPT_BsdSocketFdReference m_SocketFdReference;
    NPT_SocketInfo           m_Info;
    bool                     m_Blocking;
};

/*----------------------------------------------------------------------
|   NPT_BsdSocket::NPT_BsdSocket
+---------------------------------------------------------------------*/
NPT_BsdSocket::NPT_BsdSocket(SocketFd fd, bool force_blocking) : 
    m_SocketFdReference(new NPT_BsdSocketFd(fd)),
    m_Blocking(true)
{
    if (force_blocking) m_SocketFdReference->SetBlockingMode(true);
    RefreshInfo();
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::~NPT_BsdSocket
+---------------------------------------------------------------------*/
NPT_BsdSocket::~NPT_BsdSocket()
{
    // release the socket fd reference
    m_SocketFdReference = NULL;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::Bind
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::Bind(const NPT_SocketAddress& address, bool reuse_address)
{
    // set socket options
    if (reuse_address) {
        int option = 1;
        setsockopt(m_SocketFdReference->m_SocketFd, 
                   SOL_SOCKET, 
                   SO_REUSEADDR, 
                   (SocketOption)&option, 
                   sizeof(option));
    }
    
    // convert the address
    struct sockaddr_in inet_address;
    SocketAddressToInetAddress(address, &inet_address);
    
    // bind the socket
    if (bind(m_SocketFdReference->m_SocketFd, 
             (struct sockaddr*)&inet_address, 
             sizeof(inet_address)) < 0) {
        return NPT_ERROR_BIND_FAILED;
    }

    // refresh socket info
    RefreshInfo();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::Connect
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdSocket::Connect(const NPT_SocketAddress&, NPT_Timeout)
{
    // this is unsupported unless overridden in a derived class
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::WaitForConnection
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdSocket::WaitForConnection(NPT_Timeout)
{
    // this is unsupported unless overridden in a derived class
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::GetInputStream(NPT_InputStreamReference& stream)
{
    // default value
    stream = NULL;

    // check that we have a socket
    if (m_SocketFdReference.IsNull()) return NPT_ERROR_INVALID_STATE;

    // create a stream
    stream = new NPT_BsdSocketInputStream(m_SocketFdReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::GetOutputStream(NPT_OutputStreamReference& stream)
{
    // default value
    stream = NULL;

    // check that the file is open
    if (m_SocketFdReference.IsNull()) return NPT_ERROR_INVALID_STATE;

    // create a stream
    stream = new NPT_BsdSocketOutputStream(m_SocketFdReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::GetInfo(NPT_SocketInfo& info)
{
    // return the cached info
    info = m_Info;
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::RefreshInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::RefreshInfo()
{
    // check that we have a socket
    if (m_SocketFdReference.IsNull()) return NPT_ERROR_INVALID_STATE;

    // get the local socket addr
    struct sockaddr_in inet_address;
    socklen_t          name_length = sizeof(inet_address);
    if (getsockname(m_SocketFdReference->m_SocketFd, 
                    (struct sockaddr*)&inet_address, 
                    &name_length) == 0) {
        m_Info.local_address.SetIpAddress(ntohl(inet_address.sin_addr.s_addr));
        m_Info.local_address.SetPort(ntohs(inet_address.sin_port));
    }   

    // get the peer socket addr
    if (getpeername(m_SocketFdReference->m_SocketFd,
                    (struct sockaddr*)&inet_address, 
                    &name_length) == 0) {
        m_Info.remote_address.SetIpAddress(ntohl(inet_address.sin_addr.s_addr));
        m_Info.remote_address.SetPort(ntohs(inet_address.sin_port));
    }   

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::SetBlockingMode
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::SetBlockingMode(bool blocking)
{
    if (m_Blocking != blocking) {
        m_SocketFdReference->SetBlockingMode(blocking);
        m_Blocking = blocking;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::SetReadTimeout
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::SetReadTimeout(NPT_Timeout timeout)
{
    m_SocketFdReference->m_ReadTimeout = timeout;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdSocket::SetWriteTimeout
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdSocket::SetWriteTimeout(NPT_Timeout timeout)
{
    m_SocketFdReference->m_WriteTimeout = timeout;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Socket::~NPT_Socket
+---------------------------------------------------------------------*/
NPT_Socket::~NPT_Socket()
{
    delete m_SocketDelegate;
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpSocket
+---------------------------------------------------------------------*/
class NPT_BsdUdpSocket : public    NPT_UdpSocketInterface,
                         protected NPT_BsdSocket
                         
{
 public:
    // constructor
     NPT_BsdUdpSocket();
    virtual ~NPT_BsdUdpSocket() {}

    // NPT_SocketInterface methods
    NPT_Result Connect(const NPT_SocketAddress& address,
                       NPT_Timeout              timeout);

    // NPT_UdpSocketInterface methods
    NPT_Result Send(const NPT_DataBuffer&    packet, 
                    const NPT_SocketAddress* address);
    NPT_Result Receive(NPT_DataBuffer&     packet, 
                       NPT_SocketAddress*  address);

    // friends
    friend class NPT_UdpSocket;
};

/*----------------------------------------------------------------------
|   NPT_BsdUdpSocket::NPT_BsdUdpSocket
+---------------------------------------------------------------------*/
NPT_BsdUdpSocket::NPT_BsdUdpSocket() : 
    NPT_BsdSocket(socket(AF_INET, SOCK_DGRAM, 0))
{
    // set default socket options
    int option = 1;
    setsockopt(m_SocketFdReference->m_SocketFd, 
               SOL_SOCKET, 
               SO_BROADCAST, 
               (SocketOption)&option, 
               sizeof(option));
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpSocket::Connect
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpSocket::Connect(const NPT_SocketAddress& address, 
                          NPT_Timeout /* ignored */)
{
    // setup an address structure
    struct sockaddr_in inet_address;
    SocketAddressToInetAddress(address, &inet_address);

    // connect so that we can have some addr bound to the socket
    int io_result = connect(m_SocketFdReference->m_SocketFd, 
                            (struct sockaddr *)&inet_address, 
                            sizeof(inet_address));
    if (NPT_BSD_SOCKET_CALL_FAILED(io_result)) { 
        return MapErrorCode(GetSocketError());
    }
    
    // refresh socket info
    RefreshInfo();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpSocket::Send
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdUdpSocket::Send(const NPT_DataBuffer&    packet, 
                       const NPT_SocketAddress* address) 
{
    // get the packet buffer
    const NPT_Byte* buffer        = packet.GetData();
    ssize_t         buffer_length = packet.GetDataSize();

    // if we're blocking, wait until the socket is writeable
    if (m_SocketFdReference->m_Blocking && 
        m_SocketFdReference->m_WriteTimeout != NPT_TIMEOUT_INFINITE) {
        NPT_Result result = m_SocketFdReference->WaitUntilWriteable();
        if (result != NPT_SUCCESS) return result;
    }

    // send the packet buffer
    int io_result;
    if (address) {
        // send to the specified address

        // setup an address structure
        struct sockaddr_in inet_address;
        SocketAddressToInetAddress(*address, &inet_address);
        io_result = sendto(m_SocketFdReference->m_SocketFd, 
                           (SocketConstBuffer)buffer, 
                           buffer_length, 
                           0, 
                           (struct sockaddr *)&inet_address, 
                           sizeof(inet_address));
    } else {
        // send to whichever addr the socket is connected
        io_result = send(m_SocketFdReference->m_SocketFd, 
                         (SocketConstBuffer)buffer, 
                         buffer_length,
                         0);
    }

    // check result
    if (NPT_BSD_SOCKET_CALL_FAILED(io_result)) {
        return MapErrorCode(GetSocketError());
    }

    m_SocketFdReference->m_Position += buffer_length;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpSocket::Receive
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdUdpSocket::Receive(NPT_DataBuffer&    packet, 
                          NPT_SocketAddress* address)
{
    // get the packet buffer
    NPT_Byte* buffer      = packet.UseData();
    ssize_t   buffer_size = packet.GetBufferSize();

    // check that we have some space to receive
    if (buffer_size == 0) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // if we're blocking, wait until the socket is readable
    if (m_SocketFdReference->m_Blocking && 
        m_SocketFdReference->m_ReadTimeout != NPT_TIMEOUT_INFINITE) {
        NPT_Result result = m_SocketFdReference->WaitUntilReadable();
        if (result != NPT_SUCCESS) return result;
    }

    // receive a packet
    int io_result;
    if (address) {
        struct sockaddr_in inet_address;
        socklen_t          inet_address_length = sizeof(inet_address);
        io_result = recvfrom(m_SocketFdReference->m_SocketFd, 
                             (SocketBuffer)buffer, 
                             buffer_size, 
                             0, 
                             (struct sockaddr *)&inet_address, 
                             &inet_address_length);

        // convert the address format
        if (!NPT_BSD_SOCKET_CALL_FAILED(io_result)) {
            if (inet_address_length == sizeof(inet_address)) {
                InetAddressToSocketAddress(&inet_address, *address);
            }
        }
    } else {
        io_result = recv(m_SocketFdReference->m_SocketFd,
                         (SocketBuffer)buffer,
                         buffer_size,
                         0);
    }

    // check result
    if (NPT_BSD_SOCKET_CALL_FAILED(io_result)) {
        packet.SetDataSize(0);
        return MapErrorCode(GetSocketError());
    } 

    packet.SetDataSize(io_result);
    m_SocketFdReference->m_Position += io_result;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::NPT_UdpSocket()
{
    NPT_BsdUdpSocket* delegate = new NPT_BsdUdpSocket();
    m_SocketDelegate    = delegate;
    m_UdpSocketDelegate = delegate;
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::NPT_UdpSocket(NPT_UdpSocketInterface* delegate) :
    m_UdpSocketDelegate(delegate)
{
}

/*----------------------------------------------------------------------
|   NPT_UdpSocket::~NPT_UdpSocket
+---------------------------------------------------------------------*/
NPT_UdpSocket::~NPT_UdpSocket()
{
    delete m_UdpSocketDelegate;

    // set the delegate pointers to NULL because it is shared by the
    // base classes, and we only want to delete the object once
    m_UdpSocketDelegate = NULL;
    m_SocketDelegate    = NULL;
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket
+---------------------------------------------------------------------*/
class NPT_BsdUdpMulticastSocket : public    NPT_UdpMulticastSocketInterface,
                                  protected NPT_BsdUdpSocket
                                  
{
 public:
    // methods
     NPT_BsdUdpMulticastSocket();
    ~NPT_BsdUdpMulticastSocket();

    // NPT_UdpMulticastSocketInterface methods
    NPT_Result JoinGroup(const NPT_IpAddress& group,
                         const NPT_IpAddress& iface);
    NPT_Result LeaveGroup(const NPT_IpAddress& group,
                          const NPT_IpAddress& iface);
    NPT_Result SetTimeToLive(unsigned char ttl);
    NPT_Result SetInterface(const NPT_IpAddress& iface);

    // friends 
    friend class NPT_UdpMulticastSocket;
};

/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::NPT_BsdUdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_BsdUdpMulticastSocket::NPT_BsdUdpMulticastSocket()
{
}

/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::~NPT_BsdUdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_BsdUdpMulticastSocket::~NPT_BsdUdpMulticastSocket()
{
}

#if defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::JoinGroup
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::JoinGroup(const NPT_IpAddress& group,
                                     const NPT_IpAddress& iface)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::JoinGroup
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::JoinGroup(const NPT_IpAddress& group,
                                     const NPT_IpAddress& iface)
{
    struct ip_mreq mreq;

    // set the interface address
    mreq.imr_interface.s_addr = htonl(iface.AsLong());

    // set the group address
    mreq.imr_multiaddr.s_addr = htonl(group.AsLong());

    // set socket option
    int io_result = setsockopt(m_SocketFdReference->m_SocketFd, 
                               IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                               (SocketOption)&mreq, sizeof(mreq));
    if (io_result == 0) {
        return NPT_SUCCESS;
    } else {
        return MapErrorCode(GetSocketError());
    }
}
#endif

#if defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::LeaveGroup
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::LeaveGroup(const NPT_IpAddress& group,
                                      const NPT_IpAddress& iface)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::LeaveGroup
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::LeaveGroup(const NPT_IpAddress& group,
                                      const NPT_IpAddress& iface)
{
    struct ip_mreq mreq;

    // set the interface address
    mreq.imr_interface.s_addr = htonl(iface.AsLong());

    // set the group address
    mreq.imr_multiaddr.s_addr = htonl(group.AsLong());

    // set socket option
    int io_result = setsockopt(m_SocketFdReference->m_SocketFd, 
                               IPPROTO_IP, IP_DROP_MEMBERSHIP, 
                               (SocketOption)&mreq, sizeof(mreq));
    if (io_result == 0) {
        return NPT_SUCCESS;
    } else {
        return MapErrorCode(GetSocketError());
    }
}
#endif

#if defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::SetInterface
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::SetInterface(const NPT_IpAddress& iface)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::SetInterface
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::SetInterface(const NPT_IpAddress& iface)
{
    struct in_addr iface_addr;
    // set the interface address
    iface_addr.s_addr = htonl(iface.AsLong());

    // set socket option
    int io_result = setsockopt(m_SocketFdReference->m_SocketFd, 
                               IPPROTO_IP, IP_MULTICAST_IF, 
                               (char*)&iface_addr, sizeof(iface_addr));
    if (io_result == 0) {
        return NPT_SUCCESS;
    } else {
        return MapErrorCode(GetSocketError());
    }
}
#endif

#if defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::SetTimeToLive
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::SetTimeToLive(unsigned char ttl)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#else
/*----------------------------------------------------------------------
|   NPT_BsdUdpMulticastSocket::SetTimeToLive
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdUdpMulticastSocket::SetTimeToLive(unsigned char ttl)
{
    unsigned char ttl_opt = ttl;

    // set socket option
    int io_result = setsockopt(m_SocketFdReference->m_SocketFd, 
                               IPPROTO_IP, IP_MULTICAST_TTL, 
                               (SocketOption)&ttl_opt, sizeof(ttl_opt));
    if (io_result == 0) {
        return NPT_SUCCESS;
    } else {
        return MapErrorCode(GetSocketError());
    }
}
#endif

/*----------------------------------------------------------------------
|   NPT_UdpMulticastSocket::NPT_UdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_UdpMulticastSocket::NPT_UdpMulticastSocket() :
    NPT_UdpSocket(NULL)
{
    NPT_BsdUdpMulticastSocket* delegate = new NPT_BsdUdpMulticastSocket();
    m_SocketDelegate             = delegate;
    m_UdpSocketDelegate          = delegate;
    m_UdpMulticastSocketDelegate = delegate;
}

/*----------------------------------------------------------------------
|   NPT_UdpMulticastSocket::~NPT_UdpMulticastSocket
+---------------------------------------------------------------------*/
NPT_UdpMulticastSocket::~NPT_UdpMulticastSocket()
{
    delete m_UdpMulticastSocketDelegate;

    // set the delegate pointers to NULL because it is shared by the
    // base classes, and we only want to delete the object once
    m_SocketDelegate             = NULL;
    m_UdpSocketDelegate          = NULL;
    m_UdpMulticastSocketDelegate = NULL;
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket
+---------------------------------------------------------------------*/
class NPT_BsdTcpClientSocket : protected NPT_BsdSocket
{
 public:
    // methods
     NPT_BsdTcpClientSocket();
    ~NPT_BsdTcpClientSocket();

    // NPT_SocketInterface methods
    NPT_Result Connect(const NPT_SocketAddress& address,
                       NPT_Timeout              timeout);
    NPT_Result WaitForConnection(NPT_Timeout timeout);

protected:
    // methods
    NPT_Result DoWaitForConnection(NPT_Timeout timeout);

    // friends
    friend class NPT_TcpClientSocket;
};

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket::NPT_BsdTcpClientSocket
+---------------------------------------------------------------------*/
NPT_BsdTcpClientSocket::NPT_BsdTcpClientSocket() : 
    NPT_BsdSocket(socket(AF_INET, SOCK_STREAM, 0))
{
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket::~NPT_BsdTcpClientSocket
+---------------------------------------------------------------------*/
NPT_BsdTcpClientSocket::~NPT_BsdTcpClientSocket()
{
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket::Connect
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdTcpClientSocket::Connect(const NPT_SocketAddress& address, 
                                NPT_Timeout              timeout)
{
    SocketFd socket_fd = m_SocketFdReference->m_SocketFd;
    bool was_blocking = m_Blocking;

    // set the socket to nonblocking so that we can timeout on connect
    if (m_Blocking) {
        NPT_CHECK(m_SocketFdReference->SetBlockingMode(false));
    }

    // convert the address
    struct sockaddr_in inet_address;
    SocketAddressToInetAddress(address, &inet_address);

    // initiate connection
    int io_result;
    io_result = connect(socket_fd, 
                        (struct sockaddr *)&inet_address, 
                        sizeof(inet_address));
    if (io_result == 0) {
        // immediate connection

        // put the fd back in its original blocking mode
        if (was_blocking) m_SocketFdReference->SetBlockingMode(true);

        // get socket info
        RefreshInfo();

        return NPT_SUCCESS;
    }

    NPT_Result result = MapErrorCode(GetSocketError());
    
    if (was_blocking) {
        // put the fd back in its original blocking mode
        m_SocketFdReference->SetBlockingMode(true);

        // wait for the connection to be done
        if (result == NPT_ERROR_WOULD_BLOCK) return DoWaitForConnection(timeout);
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket::WaitForConnection
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdTcpClientSocket::WaitForConnection(NPT_Timeout timeout)
{
    // this function can only be called directly for non-blocking sockets
    if (m_Blocking) return NPT_ERROR_INVALID_STATE;

    return DoWaitForConnection(timeout);
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpClientSocket::DoWaitForConnection
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BsdTcpClientSocket::DoWaitForConnection(NPT_Timeout timeout)
{
    SocketFd   socket_fd = m_SocketFdReference->m_SocketFd;
    NPT_Result result = NPT_SUCCESS;

    // wait for connection to succeed or fail
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;
    FD_ZERO(&read_set);
    FD_SET(socket_fd, &read_set);
    FD_ZERO(&write_set);
    FD_SET(socket_fd, &write_set);
    FD_ZERO(&except_set);
    FD_SET(socket_fd, &except_set);

    struct timeval timeout_value;
    if (timeout != NPT_TIMEOUT_INFINITE) {
        timeout_value.tv_sec = timeout/1000;
        timeout_value.tv_usec = 1000*(timeout-1000*(timeout/1000));
    };

    int io_result = select((int)socket_fd+1, 
                           &read_set, &write_set, &except_set, 
                           timeout == NPT_TIMEOUT_INFINITE ? 
                           NULL : &timeout_value);

    if (io_result == 0) {
        if (timeout == 0) {
            // non-blocking call
            return NPT_ERROR_WOULD_BLOCK;
        } else {
            // timeout
            return NPT_ERROR_TIMEOUT;
        }
    } else if (NPT_BSD_SOCKET_SELECT_FAILED(io_result)) {
        return MapErrorCode(GetSocketError());
    } else if (FD_ISSET(socket_fd, &read_set)   || 
               FD_ISSET(socket_fd, &write_set)  ||
               FD_ISSET(socket_fd, &except_set)) {
#if defined(_XBOX)
        if (FD_ISSET(socket_fd, &except_set)) return MapErrorCode(GetSocketError());
#else
        // get error status from socket
        // (some systems return the error in errno, others
        //  return it in the buffer passed to getsockopt)
        int error = 0;
        socklen_t length = sizeof(error);
        io_result = getsockopt(socket_fd, 
                               SOL_SOCKET, 
                               SO_ERROR, 
                               (SocketOption)&error, 
                               &length);
        if (NPT_BSD_SOCKET_CALL_FAILED(io_result)) {
            return MapErrorCode(GetSocketError());
        } else if (error) {
            return MapErrorCode(error);
        }
#endif
    } else {
        // should not happen
        return NPT_ERROR_INTERNAL;
    }
    
    // get socket info
    RefreshInfo();

    return result;
}

/*----------------------------------------------------------------------
|   NPT_TcpClientSocket::NPT_TcpClientSocket
+---------------------------------------------------------------------*/
NPT_TcpClientSocket::NPT_TcpClientSocket() :
    NPT_Socket(new NPT_BsdTcpClientSocket())
{
}

/*----------------------------------------------------------------------
|   NPT_TcpClientSocket::NPT_TcpClientSocket
+---------------------------------------------------------------------*/
NPT_TcpClientSocket::~NPT_TcpClientSocket()
{
    delete m_SocketDelegate;

    // set the delegate pointer to NULL because it is shared by the
    // base classes, and we only want to delete the object once
    m_SocketDelegate = NULL;
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpServerSocket
+---------------------------------------------------------------------*/
class NPT_BsdTcpServerSocket : public    NPT_TcpServerSocketInterface,
                               protected NPT_BsdSocket
                               
{
 public:
    // methods
     NPT_BsdTcpServerSocket();
    ~NPT_BsdTcpServerSocket();

    // NPT_SocketInterface methods
    NPT_Result Bind(const NPT_SocketAddress& address, bool reuse_address = true) {
        // inherit
        return NPT_BsdSocket::Bind(address, reuse_address);
    }
    NPT_Result Connect(const NPT_SocketAddress& address,
                       NPT_Timeout              timeout) {
        // inherit
        return NPT_BsdSocket::Connect(address, timeout);
    }
    NPT_Result GetInputStream(NPT_InputStreamReference& stream) {
        // no stream
        stream = NULL;
        return NPT_ERROR_NOT_SUPPORTED;
    }
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) {
        // no stream
        stream = NULL;
        return NPT_ERROR_NOT_SUPPORTED;
    }
    NPT_Result GetInfo(NPT_SocketInfo& info) {
        // inherit
        return NPT_BsdSocket::GetInfo(info);
    }

    // NPT_TcpServerSocket methods
    NPT_Result Listen(unsigned int max_clients);
    NPT_Result WaitForNewClient(NPT_Socket*& client, NPT_Timeout timeout);

protected:
    // members
    unsigned int m_ListenMax;

    // friends
    friend class NPT_TcpServerSocket;
};

/*----------------------------------------------------------------------
|   NPT_BsdTcpServerSocket::NPT_BsdTcpServerSocket
+---------------------------------------------------------------------*/
NPT_BsdTcpServerSocket::NPT_BsdTcpServerSocket() : 
    NPT_BsdSocket(socket(AF_INET, SOCK_STREAM, 0)),
    m_ListenMax(0)
{
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpServerSocket::~NPT_BsdTcpServerSocket
+---------------------------------------------------------------------*/
NPT_BsdTcpServerSocket::~NPT_BsdTcpServerSocket()
{
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpServerSocket::Listen
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdTcpServerSocket::Listen(unsigned int max_clients)
{
    // listen for connections
    if (listen(m_SocketFdReference->m_SocketFd, max_clients) < 0) {
        m_ListenMax = 0;
        return NPT_ERROR_LISTEN_FAILED;
    }   
    m_ListenMax = max_clients;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BsdTcpServerSocket::WaitForNewClient
+---------------------------------------------------------------------*/
NPT_Result
NPT_BsdTcpServerSocket::WaitForNewClient(NPT_Socket*& client, 
                                         NPT_Timeout  timeout)
{
    // default value
    client = NULL;

    // check that we are listening for clients
    if (m_ListenMax == 0) {
        Listen(NPT_TCP_SERVER_SOCKET_DEFAULT_LISTEN_COUNT);
    }

    // set the socket to nonblocking so that we can timeout on accept
    bool was_blocking = m_Blocking;
    if (m_Blocking) {
        NPT_CHECK(m_SocketFdReference->SetBlockingMode(false));
    }

    // wait for incoming connection
    SocketFd   socket_fd = m_SocketFdReference->m_SocketFd;
    NPT_Result result = NPT_SUCCESS;
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;
    FD_ZERO(&read_set);
    FD_SET(socket_fd, &read_set);
    FD_ZERO(&write_set);
    FD_SET(socket_fd, &write_set);
    FD_ZERO(&except_set);
    FD_SET(socket_fd, &except_set);

    struct timeval timeout_value;
    if (timeout != NPT_TIMEOUT_INFINITE) {
        timeout_value.tv_sec = timeout/1000;
        timeout_value.tv_usec = 1000*(timeout-1000*(timeout/1000));
    };
    
    int io_result = select((int)m_SocketFdReference->m_SocketFd+1, 
                            &read_set, &write_set, &except_set, 
                            timeout == NPT_TIMEOUT_INFINITE ? 
                            NULL : &timeout_value);

    if (io_result == 0) {
        if (timeout == 0) {
            // non-blocking call
            result = NPT_ERROR_WOULD_BLOCK;
        } else {
            // timeout
            result = NPT_ERROR_TIMEOUT;
        }
    } else if (NPT_BSD_SOCKET_SELECT_FAILED(io_result)) {
        result = MapErrorCode(GetSocketError());
    } else if (FD_ISSET(socket_fd, &read_set) || 
               FD_ISSET(socket_fd, &write_set)) {
        struct sockaddr_in inet_address;
        socklen_t          namelen = sizeof(inet_address);
        socket_fd = accept(socket_fd, (struct sockaddr*)&inet_address, &namelen); 
        if (NPT_BSD_SOCKET_IS_INVALID(socket_fd)) {
            result = MapErrorCode(GetSocketError());
        } else {
            client = new NPT_Socket(new NPT_BsdSocket(socket_fd, was_blocking));
        }
    } else if (FD_ISSET(socket_fd, &except_set)) {
        result = MapErrorCode(GetSocketError());
    } else {
        // should not happen
        result = NPT_ERROR_INTERNAL;
    }

    if (was_blocking) {
        // put the fd back in its original blocking mode
        m_SocketFdReference->SetBlockingMode(true);
    }

    // done
    return result;    
}

/*----------------------------------------------------------------------
|   NPT_TcpServerSocket::NPT_TcpServerSocket
+---------------------------------------------------------------------*/
NPT_TcpServerSocket::NPT_TcpServerSocket()
{
    NPT_BsdTcpServerSocket* delegate = new NPT_BsdTcpServerSocket();
    m_SocketDelegate          = delegate;
    m_TcpServerSocketDelegate = delegate;
}

/*----------------------------------------------------------------------
|   NPT_TcpServerSocket::NPT_TcpServerSocket
+---------------------------------------------------------------------*/
NPT_TcpServerSocket::~NPT_TcpServerSocket()
{
    delete m_TcpServerSocketDelegate;

    // set the delegate pointers to NULL because it is shared by the
    // base classes, and we only want to delete the object once
    m_SocketDelegate          = NULL;
    m_TcpServerSocketDelegate = NULL;
}
