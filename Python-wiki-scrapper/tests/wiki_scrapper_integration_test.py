import sys
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parent.parent))

from wiki_scrapper import WikiScrapper

def test_summary():
    scrapper = WikiScrapper("")
    scrapper.scrape(local_file="tests/Team_Rocket.html")
    phrase = scrapper.get_summary()

    assert("Team Rocket" == phrase[0 : 11])
    assert("the Sevii Islands.\n" == phrase[-19:])


if __name__ == "__main__":
    test_summary()