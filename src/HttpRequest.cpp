#include "HttpRequest.hpp"
#include <string>


std::atomic<bool> HttpRequest::s_isGloballyInitialized{false};

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

static size_t headerCallback(char* buffer, size_t size,
    size_t nitems, void* userdata)
{
    std::string *headers = (std::string*) userdata;
    headers->append(buffer, nitems * size);
    return nitems * size;
}

HttpRequest::HttpRequest(const std::string& url) : m_curlPtr(curl_easy_init())
{
    if (!m_curlPtr)
    {
        std::cout << "curl_easy_init failed !! \n";
        return;
    }
    if(!s_isGloballyInitialized)
    {
        initHttpRequestsGlobal();
    }

    //! send to url
    curl_easy_setopt(m_curlPtr, CURLOPT_URL, url.c_str());
    // //! add header in response string
    // curl_easy_setopt(m_curlPtr, CURLOPT_HEADER, 1L);
    //! HttpRequest::writeFunction get's called with response data
    curl_easy_setopt(m_curlPtr, CURLOPT_WRITEFUNCTION, writeFunction);
    //! data inserted inside m_responseString
    curl_easy_setopt(m_curlPtr, CURLOPT_WRITEDATA, &m_responseString);
    //! call back for header in response
    curl_easy_setopt(m_curlPtr, CURLOPT_HEADERFUNCTION, headerCallback);
    //! store response's header in m_responseHeaderString
    curl_easy_setopt(m_curlPtr, CURLOPT_HEADERDATA, &m_responseHeaderString);
    //! curl version
    curl_easy_setopt(m_curlPtr, CURLOPT_USERAGENT, std::string(std::string("curl/") + curl_version_info(CURLVERSION_NOW)->version).c_str());
    //! HTTP version
    curl_easy_setopt(m_curlPtr, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    std::cout << "new request client\n";
}

void HttpRequest::initHttpRequestsGlobal()
{
    if (!s_isGloballyInitialized)
    {
        std::cout << "Global init !!\n";
        curl_global_init(CURL_GLOBAL_DEFAULT);
        s_isGloballyInitialized = true;
    }
}

void HttpRequest::cleanHttpRequestsGlobal()
{
    if(s_isGloballyInitialized)
        curl_global_cleanup();
    s_isGloballyInitialized = false;
}

void HttpRequest::addDataToHeader(const std::string& data)
{
    //! this returns a linked list
    //! assign the net ll to headers list
    m_headers = curl_slist_append(m_headers, data.c_str());
}

void HttpRequest::addJWTtokenToHeader(const std::string& jwtToken)
{
    addDataToHeader("Authorization: Bearer " + jwtToken);
}

CURLcode HttpRequest::send()
{
    // applyHeaders
    curl_easy_setopt(m_curlPtr, CURLOPT_HTTPHEADER, m_headers);
 
    //! Perform the request, res will get the return code
    CURLcode res = curl_easy_perform(m_curlPtr);
 
    //! Check for errors
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    return res;
}

const std::string HttpRequest::getResponse()
{
    return m_responseString;
}

const std::string HttpRequest::getResponseHeader()
{
    return m_responseHeaderString;
}

HttpRequest::~HttpRequest()
{
    curl_easy_cleanup(m_curlPtr);
    curl_slist_free_all(m_headers);
}