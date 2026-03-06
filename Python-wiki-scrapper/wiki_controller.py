import re
import time
import os
import json
from collections import Counter

import matplotlib.pyplot as plt
import pandas as pd
from wordfreq import word_frequency, top_n_list

from wiki_errors import InvalidArgumentError
from wiki_scrapper import WikiScrapper


class WikiController:
    def __init__(self, wiki_url):
        self.wiki_url = wiki_url
        self.scrapper = WikiScrapper(wiki_url)
        self.visited_phrases = set()

    def summary(self, phrase):
        self.scrapper.scrape(phrase=phrase)
        print(self.scrapper.get_summary())

    def table(self, phrase, number, first_row_header=False):
        self.scrapper.scrape(phrase=phrase)
        pandas_table = self.scrapper.get_table(number, first_row_header)
        
        filename = f"{phrase}.csv"
        pandas_table.to_csv(filename, encoding='utf-8')

        raw_data = pandas_table.iloc[:, 1:].to_numpy().flatten()
        series = pd.Series(raw_data)
        
      
        counts_table = series.value_counts().reset_index()
        counts_table.columns = ["Value", "Count"]
        
        print(counts_table)
        
    def count_words(self, phrase):
        self.scrapper.scrape(phrase=phrase)
        phrase_content = self.scrapper.get_words()
        phrase_content = phrase_content.lower()
        word_list = re.findall(r"\b[a-z]+(?:'[a-z]+)?\b", phrase_content)
         
        current_counts = Counter(word_list)
        counts_path = "./word-counts.json"
        all_counts = Counter()

        if os.path.exists(counts_path):
            with open(counts_path) as f:
                data = json.load(f)
                all_counts = Counter(data)
           
        all_counts.update(current_counts)
        with open(counts_path, 'w') as f:
            json.dump(dict(all_counts), f, ensure_ascii=False, indent=2)

    def analyze_relative_word_frequency(self, mode, count, chart=None):
        language = "en"

        counts_path = "./word-counts.json"
        if os.path.exists(counts_path):
             with open(counts_path) as f:
                data = json.load(f)
                data = Counter(data)
        else:
            raise InvalidArgumentError("There is no words to sample from.")

        if not isinstance(count, int) or count < 0:
            raise InvalidArgumentError("Please provide correct count value.")

        article_most_common = data.most_common(count)
        language_most_common = top_n_list(language, count)

        word_data = []
        if mode == "article":
            for word, count in article_most_common:
                lang_freq = word_frequency(word, language)
                word_data.append({
                    'word': word,
                    'wiki_count': count,
                    'lang_freq': lang_freq
                })
        elif mode == "language":
            for word in language_most_common:
                lang_freq = word_frequency(word, language)
                wiki_count = data.get(word, 0)

                word_data.append({
                    'word': word,
                    'wiki_count': wiki_count,
                    'lang_freq': lang_freq
                })
        else:
            raise InvalidArgumentError("Invalid mode argument given.")
        
        word_table = pd.DataFrame(word_data)
        max_wiki = word_table['wiki_count'].max()
        max_language = word_table['lang_freq'].max()

        word_table['lang_freq'] = word_table['lang_freq'] / max_language
        word_table['wiki_freq'] = word_table['wiki_count'] / max_wiki
        result_table = word_table[['word', 'wiki_freq', 'lang_freq']]

        print(result_table)

        if chart:
            self.generate_chart(result_table, chart, language)
    
    def generate_chart(self, data, path, language):
        plot_data = data.set_index('word')
        
        plot_data.plot(kind='bar', figsize=(10, 6), width=0.8)
        
        plt.title("Frequency of words: Wiki vs Language")
        plt.ylabel("Normalized Frequency (0-1)")
        plt.xlabel("Words")
        plt.legend(["Wiki Article", f"{language}"])
        plt.tight_layout() 
        
        
        plt.savefig(path)
        plt.close() 

    def auto_count_words(self, phrase, n, t):
        if not isinstance(n, int) or n < 0:
            raise InvalidArgumentError("Provide correct depth value.")
        
        if not isinstance(t, int) or t < 0:
            raise InvalidArgumentError("Provide correct time value.")

        self.auto_count_words_execute(phrase, n, t)
    
    def auto_count_words_execute(self, current_phrase, n, t):
        print(current_phrase)
        self.count_words(phrase=current_phrase)
        self.visited_phrases.add(current_phrase)
        time.sleep(t)

        if n > 0:   
            all_links_and_phrases = self.scrapper.get_all_links()

            for _, phrase in all_links_and_phrases:
                if phrase not in self.visited_phrases:
                    self.auto_count_words_execute(phrase, n - 1, t)            

if __name__ == "__main__":
    try:
        controller = WikiController("https://bulbapedia.bulbagarden.net/wiki")
        controller.table("Type", 1, True)
        controller.count_words("charizard")
        controller.auto_count_words("Chain breeding", 1, 1)
        controller.analyze_relative_word_frequency("article", 5, "analisys")
    except Exception as e:
        print(f"Error: {e}")