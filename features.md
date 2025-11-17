More Features(Industry Standard)


1. 🚀 Core Search & Indexing

These are the fundamental features that define your database's performance and accuracy.


    Vector Compression (Quantization): This is a key feature for saving memory. Product Quantization (PQ) is a common technique that compresses large vectors into smaller ones, allowing you to fit billions of vectors into memory at the cost of some accuracy.



2. 🎯 Critical Database Features

These are the features that make it a true database and not just a search library.

    Advanced Metadata Filtering: This is your killer feature. It's not enough to just store metadata; users must be able to filter on it.

    Pre-filtering: "Find all vectors where year == 2024, then perform the vector search." This is accurate but can be slow if the filter is broad.

    Post-filtering: "Find the top 100 vectors, then filter out any that don't match year == 2024." This is fast but can return fewer than 100 results (or even zero).

    Operators: Support more than just equality (=). You need $gt (greater than), $lt (less than), $in (in a list), $and, $or, etc.

CRUD Operations: **Implemented.** Full support for add, get (via query), update, and delete is complete. Deletion uses a soft-delete and manual rebuild strategy.



3. ✨ Advanced "Wow" Features


        Built-in Embedding Functions: Make it easy for beginners. Integrate with common embedding libraries (like sentence-transformers) so a user can just collection.add("Hello world!") and you handle the vectorization automatically in the background.

