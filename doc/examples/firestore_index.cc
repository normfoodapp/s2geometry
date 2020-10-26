// Copyright 2017 Google Inc. All Rights Reserved.
// Author: ericv@google.com (Eric Veach)
//
// This example shows how to add spatial data to an information retrieval
// system.  Such systems work by converting documents into a collection of
// "index terms" (e.g., representing words or phrases), and then building an
// "inverted index" that maps each term to a list of documents (and document
// positions) where that term occurs.
//
// This example shows how to convert spatial data into index terms, which can
// then be indexed along with the other document information.

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <set>
#include <unordered_map>
#include <vector>

#include "s2/base/commandlineflags.h"
#include "s2/s2earth.h"
#include "s2/s2cap.h"
#include "s2/s2point_index.h"
#include "s2/s2region_term_indexer.h"
#include "s2/s2testing.h"

class S2Doc {
	public:
		S2Point docPoint;
		string docId;

	S2Doc(S2Point _docPoint, string _docId) {
		docPoint = _docPoint;
		docId = _docId;
	}
};

S2_DEFINE_int32(num_documents, 100, "Number of documents");
S2_DEFINE_int32(num_queries, 1, "Number of queries");
S2_DEFINE_double(query_radius_km, 100, "Query radius in kilometers");

// A prefix added to spatial terms to distinguish them from other index terms
// (e.g. representing words or phrases).
static const char kPrefix[] = "s2:";

int main(int argc, char **argv) {
  // Create a set of "documents" to be indexed.  Each document consists of a
  // single point.  (You can easily substitute any S2Region type here, or even
  // index a mixture of region types using std::unique_ptr<S2Region>.  Other
  // region types include polygons, polylines, rectangles, discs, buffered
  // geometry, etc.)
  std::vector<S2Doc> documents;
  //std::vector<S2Point> documents;
  documents.reserve(FLAGS_num_documents);
  for (int docid = 0; docid < FLAGS_num_documents; ++docid) {
    //documents.push_back(S2Testing::RandomPoint());
	S2Point s2point(S2LatLng::FromDegrees(40.2375772, -111.6777521));
    documents.push_back(S2Doc(s2point, "firestore_"+std::to_string(docid)));
  }

  // We use a hash map as our inverted index.  The key is an index term, and
  // the value is the set of "document ids" where this index term is present.
  std::unordered_map<string, std::vector<int>> index;

  // Create an indexer suitable for an index that contains points only.
  // (You may also want to adjust min_level() or max_level() if you plan
  // on querying very large or very small regions.)
  S2RegionTermIndexer::Options options;
  options.set_index_contains_points_only(true);
  S2RegionTermIndexer indexer(options);

  // Add the documents to the index.
  for (int docid = 0; docid < documents.size(); ++docid) {
    S2Point index_region = documents[docid].docPoint;
    for (const auto& term : indexer.GetIndexTerms(index_region, kPrefix)) {
      index[term].push_back(docid);
    }
  }

  // Convert the query radius to an angle representation.
  S1Angle radius =
      S1Angle::Radians(S2Earth::KmToRadians(FLAGS_query_radius_km));

  // Count the number of documents (points) found in all queries.
  int64_t num_found = 0;
  for (int i = 0; i < FLAGS_num_queries; ++i) {
    // Choose a random center for query.
    //S2Cap query_region(S2Testing::RandomPoint(), radius);
	S2Point s2point(S2LatLng::FromDegrees(40.2375772, -111.6777521));
	S2Cap query_region(s2point, radius);

    // Convert the query region to a set of terms, and compute the union of
    // the document ids associated with those terms.  (An actual information
    // retrieval system would do something more sophisticated.)
    std::set<int> candidates;
    for (const auto& term : indexer.GetQueryTerms(query_region, kPrefix)) {
      candidates.insert(index[term].begin(), index[term].end());
    }

    // "candidates" now contains all documents that intersect the query
    // region, along with some documents that nearly intersect it.  We can
    // prune the results by retrieving the original "document" and checking
    // the distance more precisely.
    std::vector<string> result;
    for (int docid : candidates) {
      if (!query_region.Contains(documents[docid].docPoint)) continue;
      result.push_back(documents[docid].docId);
    }
    // Now do something with the results (in this example we just count them).
    num_found += result.size();

    for (int resIndex = 0; resIndex < result.size(); ++resIndex) {
    	std::printf("Found document with id: %s\n", result[resIndex].c_str());
    }
  }
  std::printf("Found %" PRId64 " points in %d queries\n",
              num_found, FLAGS_num_queries);
  return  0;
}
