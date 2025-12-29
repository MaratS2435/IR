import requests
from bs4 import BeautifulSoup
from pymongo import MongoClient
import time
import random
import json
import os
import sys
import yaml
import argparse
from urllib.parse import urlparse, urlunparse


def load_config(path):
    with open(path, 'r') as f:
        return yaml.safe_load(f)


parser = argparse.ArgumentParser(description='Search Robot')
parser.add_argument('config', type=str, help='Path to config.yaml')
args = parser.parse_args()

cfg = load_config(args.config)

client = MongoClient(cfg['db']['host'], cfg['db']['port'])
db = client[cfg['db']['name']]
a_coll = db[cfg['db']['collection_articles']]
q_coll = db[cfg['db']['collection_queue']]

a_coll.create_index("url", unique=True)
q_coll.create_index("url", unique=True)

HEADERS = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36',
    'Accept-Language': 'ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7'
}


def normalize_url(url):
    parsed = urlparse(url)
    clean_url = urlunparse((parsed.scheme.lower(), parsed.netloc.lower(), parsed.path, '', '', ''))
    if clean_url.endswith('/'):
        clean_url = clean_url[:-1]
    return clean_url


def load_state():
    if os.path.exists(cfg['logic']['state_file']):
        with open(cfg['logic']['state_file'], 'r') as f:
            return json.load(f)
    return {"b17_page": 1, "psych_page": 1}


def save_state(b17_p, psych_p):
    with open(cfg['logic']['state_file'], 'w') as f:
        json.dump({"b17_page": b17_p, "psych_page": psych_p}, f)


def add_to_queue(url, source):
    url = normalize_url(url)
    existing_doc = a_coll.find_one({'url': url})

    if existing_doc:
        last_scan = existing_doc.get('timestamp', 0)
        if time.time() - last_scan < cfg['logic']['recrawl_interval']:
            return False
        else:
            print(f"Re-crawl: {url}")

    try:
        q_coll.update_one(
            {'url': url},
            {'$set': {'url': url, 'source': source, 'added_at': time.time()}},
            upsert=True
        )
        return True
    except:
        return False


def harvest_lists():
    state = load_state()
    b17_page = state['b17_page']
    psych_page = state['psych_page']

    print("Starting harvester...")

    try:
        while True:
            # B17
            try:
                r = requests.get(f"https://www.b17.ru/article/?page={b17_page}", headers=HEADERS)
                if r.status_code == 200:
                    soup = BeautifulSoup(r.text, 'lxml')
                    links = soup.find_all('a', href=True)
                    if not links:
                        break

                    count = 0
                    for a in links:
                        href = a['href'].split('#')[0]
                        if href.startswith('/article/') and href.count('/') == 3:
                            if add_to_queue("https://www.b17.ru" + href, 'b17'):
                                count += 1
                    print(f"B17 page {b17_page}: {count} added")
                    b17_page += 1
            except Exception as e:
                print(f"Error B17: {e}")

            time.sleep(1)

            # Psychologies
            try:
                r = requests.get(f"https://www.psychologies.ru/articles/{psych_page}/", headers=HEADERS)
                if r.status_code == 200:
                    soup = BeautifulSoup(r.text, 'lxml')
                    count = 0
                    for a in soup.select('a.rubric-anons_title, .rubric-anons_list a.link'):
                        href = a.get('href')
                        if href:
                            full = href if href.startswith('http') else "https://www.psychologies.ru" + href
                            if add_to_queue(full, 'psychologies'):
                                count += 1
                    print(f"Psych page {psych_page}: {count} added")
                    psych_page += 1
            except Exception as e:
                print(f"Error Psych: {e}")

            save_state(b17_page, psych_page)
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nStopped.")


def process_queue():
    print(f"Crawler started (Delay: {cfg['logic']['delay_min']}-{cfg['logic']['delay_max']}s)")

    while True:
        task = q_coll.find_one_and_delete({})
        if not task:
            print("Queue empty.")
            break

        url = task['url']
        source = task['source']

        time.sleep(random.uniform(cfg['logic']['delay_min'], cfg['logic']['delay_max']))

        try:
            r = requests.get(url, headers=HEADERS, timeout=10)
            if r.status_code == 200:
                html = r.text
                soup = BeautifulSoup(html, 'lxml')

                if source == 'b17':
                    title, text = extract_b17_data(soup)
                else:
                    title, text = extract_psych_data(soup)

                if len(text) > 200:
                    old_doc = a_coll.find_one({'url': url})
                    if old_doc and old_doc.get('clean_text') == text:
                        a_coll.update_one({'url': url}, {'$set': {'timestamp': time.time()}})
                        print(f"Unchanged: {url}")
                    else:
                        doc = {
                            'url': url,
                            'source': source,
                            'title': title,
                            'raw_html': html,
                            'clean_text': text,
                            'timestamp': time.time()
                        }
                        a_coll.replace_one({'url': url}, doc, upsert=True)
                        action = "Updated" if old_doc else "Saved"
                        print(f"{action}: {url}")
                else:
                    print(f"Short content skip: {url}")
            else:
                print(f"Status {r.status_code}: {url}")
        except Exception as e:
            print(f"Network error: {url} {e}")


def extract_b17_data(soup):
    title_tag = soup.find('h1', class_='from_bb_h1') or soup.find('h1')
    title = title_tag.get_text(strip=True) if title_tag else "No Title"
    content_div = soup.find('div', attrs={'itmprp': 'articleBody'}) or \
                  soup.find('div', attrs={'itemprop': 'articleBody'}) or \
                  soup.find('div', id='article_body')
    if content_div:
        for j in content_div.find_all('div', class_='art_start'): j.decompose()
        for t in content_div(['script', 'style']): t.decompose()
        return title, content_div.get_text(separator=' ', strip=True)
    return title, ""


def extract_psych_data(soup):
    title_tag = soup.find('h1', class_='article__title')
    title = title_tag.get_text(strip=True) if title_tag else "No Title"
    parts = []
    lead = soup.find('p', class_='article__lead-paragraph')
    if lead: parts.append(lead.get_text(strip=True))
    body = soup.find('section', attrs={'itemprop': 'articleBody'})
    if body:
        for bl in body.find_all('div', class_='article__block_type-text'):
            parts.append(bl.get_text(separator=' ', strip=True))
    return title, " ".join(parts)


if __name__ == "__main__":
    print("1. Harvest")
    print("2. Crawl")
    choice = input("Choice: ")
    if choice == '1':
        harvest_lists()
    elif choice == '2':
        process_queue()