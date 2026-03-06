import os
import io

from bs4 import BeautifulSoup
import pandas as pd
import requests

from wiki_errors import MissingArgumentError, InvalidArgumentError, PermissionDeniedError

class WikiScrapper:
    def __init__(self, wiki_url):
        self.wiki_url = wiki_url
        self.data = None

    def scrape(self, phrase=None, local_file=None):
        self.phrase = phrase
        self.local_file = local_file

        if self.phrase:
            data = self.get_phrase()
        elif self.local_file:
            data = self.open_local_file()
        else:
            raise MissingArgumentError("Provide the phrase or path name of the file.")

        self.soup = BeautifulSoup(data, 'html.parser')


    def get_phrase(self):
        prepared_phrase = self.phrase.replace(" ", "_")
        url = f"{self.wiki_url}/{prepared_phrase}"

        response = requests.get(url)

        if response.ok:
            return response.text
        else:
            if response.status_code == 404:
                raise InvalidArgumentError("Provided site doesn't exist.")
            elif response.status_code == 403:
                raise PermissionDeniedError("Permission to site denied.")
            else:
                raise ValueError("Couldn't load the website.")
    

    def open_local_file(self):
        if not os.path.exists(self.local_file):
            raise FileNotFoundError("Provided file doesn't exist.")

        with open(self.local_file, "r") as f:
            return f.read()
    
    def get_summary(self):
        if not self.soup:
            raise MissingArgumentError("Execute scrape first.")
        
        wanted_div = self.soup.find("div", class_="mw-content-ltr mw-parser-output")
        summary = wanted_div.find("p").text

        return summary
    
    def get_table(self, table_number, first_headers=False):
        if not self.soup:
            raise MissingArgumentError("Execute scrape first.")
        
        all_tables = self.soup.find_all("table")

        if table_number < 1 or table_number > len(all_tables):
            raise InvalidArgumentError("Invalid table number.")
        
        target_table = all_tables[table_number - 1]

        if first_headers:
            header_row = 0
        else:
            header_row = None
        
        try:
            pd_table = pd.read_html(io.StringIO(str(target_table)), header=header_row)
        except Exception:
            raise ValueError("Side content is damaged.")
        
        return pd_table[0]
    
    def get_words(self):
        if not self.soup:
            raise MissingArgumentError("Execute scrape first.")
        
        content_div = self.soup.find("div", class_="mw-content-ltr mw-parser-output")
        text = content_div.get_text(" ", strip=True)

        return text
    
    def get_all_links(self):
        if not self.soup:
            raise MissingArgumentError("Execute scrape first.")
        
        content_div = self.soup.find("div", class_="mw-content-ltr mw-parser-output")

        all_subpages = set()
        all_phrases_and_links = []

        for link in content_div.find_all("a", href=True):
            current_href = link["href"]

            if current_href.startswith("/wiki/"):
                if "File:" not in current_href:
                    title = link["title"]
                    all_subpages.add(current_href)
                    all_phrases_and_links.append((current_href, title))

        return all_phrases_and_links

if __name__ == "__main__":
    try:
        scraper = WikiScrapper("https://bulbapedia.bulbagarden.net/wiki")
        scraper.scrape("History of Pokémon")
        print(len(scraper.get_summary()))
        print(scraper.get_table(1))
        print(len(scraper.get_words()))
        print(scraper.get_all_links())
    except Exception as e:
        print(f"Error: {e}")