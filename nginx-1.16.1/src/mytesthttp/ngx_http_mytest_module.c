#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
 
#define PB_SIZE (1024 * 2)
#define CONTENT_TYPE "application/json;charset=GB2312"
 
 
static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);
 
static void ngx_http_read_client_request_body_handler(ngx_http_request_t *r);
 
static ngx_command_t  ngx_http_mytest_commands[] =
{
    {
        ngx_string("mytest"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
        ngx_http_mytest,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
 
    ngx_null_command
};
 
static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
    NULL,                              /* preconfiguration */
    NULL,                              /* postconfiguration */
 
    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */
 
    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */
 
    NULL,                              /* create location configuration */
    NULL                               /* merge location configuration */
};
 
ngx_module_t  ngx_http_mytest_module =
{
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,           /* module context */
    ngx_http_mytest_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};
 
static char *ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;
 
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
 
    clcf->handler = ngx_http_mytest_handler;
 
    return NGX_CONF_OK;
}
 
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
    static char uri[PB_SIZE];
    static char decode[PB_SIZE];
    static char args[PB_SIZE];
    
    char* src;
    char* dst;
    int status=NGX_HTTP_OK;
    //int reply_len=0;
    //char *reply=0;
    ngx_int_t     rc;
    ngx_chain_t   out;
    
    //post handle
    if ((r->method & (NGX_HTTP_POST|NGX_HTTP_HEAD))) 
    {
        //get body
        rc = ngx_http_read_client_request_body(r, ngx_http_read_client_request_body_handler);
        if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
        }
        return NGX_DONE;
    }
    //get handle
    else if ((r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) 
    {
        //get uri
        if (r->uri.len>=PB_SIZE)
            return NGX_HTTP_NOT_ALLOWED;    
        ngx_memcpy(uri,r->uri.data,r->uri.len);
        uri[r->uri.len]=0;
        src = uri;
        dst = decode;
        ngx_unescape_uri((u_char**)&dst, (u_char**)&src, r->uri.len, 0);
        ngx_memcpy(uri,decode,dst - decode);
        uri[dst - decode] = '\0';
    
        //get args
        if (r->args.len>=PB_SIZE)
            return NGX_HTTP_NOT_ALLOWED;
        ngx_memcpy(args,r->args.data,r->args.len);
        args[r->args.len]=0;
        src = args;
        dst =decode;
        ngx_unescape_uri((u_char**)&dst, (u_char**)&src, r->args.len, 0);
        ngx_memcpy(args,decode,dst - decode);
        args[dst - decode] = '\0';
 
        //reply=request(uri,args,&status,&reply_len);
        ngx_str_t response = ngx_string("Hello World!");
 
        if (status!=NGX_HTTP_OK)
        {
            return status;
        }
         
        ngx_str_t type =ngx_string(CONTENT_TYPE);
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_type = type;
        //r->headers_out.content_length_n = reply_len;
        r->headers_out.content_length_n = response.len;
 
        ngx_buf_t  *b = ngx_create_temp_buf(r->pool, response.len);
        if(b == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    
        ngx_memcpy(b->pos, response.data, response.len);
        b->last = b->pos+response.len;
        b->last_buf = 1;
    
        out.buf = b;
        out.next = NULL;
    
        rc = ngx_http_send_header(r);
        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        {
            return rc;
        }
 
        return ngx_http_output_filter(r, &out); 
    }
    else
    {
        return NGX_HTTP_NOT_ALLOWED;
    }   
}
 
static void ngx_http_read_client_request_body_handler(ngx_http_request_t *r)
{
    static char uri[PB_SIZE];
    static char decode[PB_SIZE];
    char* body = NULL;
    int body_size = 0;
    char* src;
    char* dst;
    //char *reply=0;
    int status=NGX_HTTP_OK;
    //int reply_len=0;
    ngx_int_t     rc;
    ngx_chain_t   out;
    
    ngx_chain_t* bufs = r->request_body->bufs;
    ngx_buf_t* buf = NULL;
    uint8_t* data_buf = NULL;
    size_t content_length = 0;
    size_t body_length = 0;
    
    //get uri
    if (r->uri.len>=PB_SIZE)
    {
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }   
    ngx_memcpy(uri,r->uri.data,r->uri.len);
    uri[r->uri.len]=0;
    src = uri;
    dst = decode;
    ngx_unescape_uri((u_char**)&dst, (u_char**)&src, r->uri.len, 0);
    ngx_memcpy(uri,decode,dst - decode);
    uri[dst - decode] = '\0';
        
    //get body
    if (r->request_body == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "reqeust_body:null");
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    } 
    if ( r->headers_in.content_length == NULL )
    {   
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "r->headers_in.content_length == NULL");
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    
    content_length = atoi( (char*)(r->headers_in.content_length->value.data) );
    data_buf = ( uint8_t* )ngx_palloc( r->pool , content_length + 1 );
    size_t buf_length = 0;
    while ( bufs )
    {
        buf = bufs->buf;
        bufs = bufs->next;
        buf_length = buf->last - buf->pos ;
        if( body_length + buf_length > content_length )
        {
            memcpy( data_buf + body_length, buf->pos, content_length - body_length);
            body_length = content_length ;
            break;
        }
        memcpy( data_buf + body_length, buf->pos, buf->last - buf->pos );
        body_length += buf->last - buf->pos;
    }
    if ( body_length )
    {
        data_buf[body_length] = 0;
    }
    body = (char *)data_buf;
    body_size = body_length;
    
    //int sequence = getSequence(r);
 
    //reply = mypost(uri, body, body_size,sequence,&status, &reply_len);
    ngx_str_t response = ngx_string("Hello World!");
    if(status != NGX_HTTP_OK)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Post failed.");
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    
    ngx_str_t type =ngx_string(CONTENT_TYPE);
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_type = type;
    //r->headers_out.content_length_n = reply_len;
    r->headers_out.content_length_n = response.len;
    
    ngx_buf_t  *b = ngx_create_temp_buf(r->pool, response.len);
    if(b == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    
    ngx_memcpy(b->pos, response.data, response.len);
    b->last = b->pos+response.len;
    b->last_buf = 1;
    
    out.buf = b;
    out.next = NULL;
    
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) 
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to do ngx_http_send_header.");
        ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
        return ;
    }
    
    ngx_http_finalize_request(r,ngx_http_output_filter(r, &out));   
    
    return;
}
