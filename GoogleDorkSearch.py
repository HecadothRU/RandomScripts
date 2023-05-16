import requests
import re
import html
from urllib.parse import quote_plus

def get_page_source(url):
    headers = {
        'User-Agent': 'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.2; .NET CLR 1.0.3705;)'
    }
    response = requests.get(url, headers=headers)
    return response.text

def get_matches(to_search, regex_pattern):
    matches = re.findall(regex_pattern, to_search)
    return matches

def is_uri(source):
    try:
        result = urlparse(source)
        return all([result.scheme, result.netloc])
    except ValueError:
        return False

def search_google(query):
    page_source = get_page_source("http://www.google.com/search?num=100&q=\"" + quote_plus(query) + "\"")
    matches = get_matches(page_source, r'url\?q=(.*?)&')

    result = []
    for match in matches:
        if match not in result and "googleusercontent" not in match and "/settings/ads" not in match:
            result.append(html.unescape(match))
    return result

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python3 script.py username")
        sys.exit(1)

    username = sys.argv[1]
    results = search_google(username)
    for result in results:
        print(result)
