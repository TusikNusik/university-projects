# Python WikiScrapper & Language Analyzer

This repository contains a Python-based web scraper and a Data Science analysis module. The project is designed to scrape articles from Wikipedia (and Fandom wikis like Bulbapedia), extract text data, and empirically evaluate a custom language detection algorithm based on word frequency lists.

---

## Features

* **Web Scraping:** Extracts text content and tables from specified wiki URLs.
* **Word Counting:** Parses HTML, cleans the text, and calculates the exact occurrences of each word (ignoring case and punctuation).
* **Language Confidence Scoring:** Calculates how well a given text matches a specific language by comparing its word frequencies against the top `k` most common words in that language (using the `wordfreq` library).
* **Empirical Analysis:** A Jupyter Notebook demonstrating the algorithm's effectiveness across different languages (English, Polish, Spanish), text types (standard wiki, "bad" wiki articles like lists/tables, and non-wiki texts), and varying `k` parameters.
* **Automated Testing:** Comprehensive unit and integration test suite using `pytest`.

---

## Project Structure

* `wiki_scrapper.py` - Core logic for downloading, parsing HTML, and extracting words/tables.
* `wiki_controller.py` - CLI controller handling user arguments (e.g., `--count-words`) and orchestrating the scraper.
* `wiki_analysis.ipynb` - Jupyter Notebook containing the empirical research, confidence score implementation, data visualization (using `pandas` and `matplotlib`), and final conclusions.
* `tests/` - Directory containing unit and integration tests.
* `requirements.txt` - List of project dependencies.
