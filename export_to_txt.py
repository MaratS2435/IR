import os
from pymongo import MongoClient

MONGO_HOST = 'localhost'
DB_NAME = 'search_engine_db'
COLLECTION_NAME = 'articles_final'
OUTPUT_DIR = 'corpus'

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

client = MongoClient(MONGO_HOST, 27017)
collection = client[DB_NAME][COLLECTION_NAME]

print("Start...")
cursor = collection.find({}, {'clean_text': 1})

count = 0
for doc in cursor:
    text = doc.get('clean_text', '')
    if not text: continue

    file_path = os.path.join(OUTPUT_DIR, f"{count}.txt")
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(text)

    count += 1
    if count % 100 == 0:
        print(f"Exported {count} documents")

print(f"Done! Total files: {count}")