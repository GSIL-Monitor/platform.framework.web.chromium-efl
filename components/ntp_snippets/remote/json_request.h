// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_JSON_REQUEST_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_JSON_REQUEST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "components/language/core/browser/url_language_histogram.h"
#include "components/ntp_snippets/remote/request_params.h"
#include "components/ntp_snippets/status.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/http/http_request_headers.h"

namespace base {
class Value;
class Clock;
}  // namespace base

namespace ntp_snippets {
class UserClassifier;

namespace internal {

// Enumeration listing all possible outcomes for fetch attempts. Used for UMA
// histograms, so do not change existing values. Insert new values at the end,
// and update the histogram definition.
enum class FetchResult {
  SUCCESS = 0,
  // DEPRECATED_EMPTY_HOSTS = 1,
  URL_REQUEST_STATUS_ERROR = 2,
  HTTP_ERROR = 3,
  JSON_PARSE_ERROR = 4,
  INVALID_SNIPPET_CONTENT_ERROR = 5,
  OAUTH_TOKEN_ERROR = 6,
  // DEPRECATED_INTERACTIVE_QUOTA_ERROR = 7,
  // DEPRECATED_NON_INTERACTIVE_QUOTA_ERROR = 8,
  MISSING_API_KEY = 9,
  RESULT_MAX = 10
};

// A single request to query remote suggestions. On success, the suggestions are
// returned in parsed JSON form (base::Value).
class JsonRequest : public net::URLFetcherDelegate {
 public:
  // A client can expect error_details only, if there was any error during the
  // fetching or parsing. In successful cases, it will be an empty string.
  using CompletedCallback =
      base::OnceCallback<void(std::unique_ptr<base::Value> result,
                              FetchResult result_code,
                              const std::string& error_details)>;

  // Builds authenticated and non-authenticated JsonRequests.
  class Builder {
   public:
    Builder();
    Builder(Builder&&);
    ~Builder();

    // Builds a Request object that contains all data to fetch new snippets.
    std::unique_ptr<JsonRequest> Build() const;

    Builder& SetAuthentication(const std::string& account_id,
                               const std::string& auth_header);
    Builder& SetCreationTime(base::TimeTicks creation_time);
    // The language_histogram borrowed from the fetcher needs to stay alive
    // until the request body is built.
    Builder& SetLanguageHistogram(
        const language::UrlLanguageHistogram* language_histogram);
    Builder& SetParams(const RequestParams& params);
    Builder& SetParseJsonCallback(ParseJSONCallback callback);
    // The clock borrowed from the fetcher will be injected into the
    // request. It will be used at build time and after the fetch returned.
    // It has to be alive until the request is destroyed.
    Builder& SetClock(base::Clock* clock);
    Builder& SetUrl(const GURL& url);
    Builder& SetUrlRequestContextGetter(
        const scoped_refptr<net::URLRequestContextGetter>& context_getter);
    Builder& SetUserClassifier(const UserClassifier& user_classifier);

    // These preview methods allow to inspect the Request without exposing it
    // publicly.
    // TODO(fhorschig): Remove these when moving the Builder to
    // snippets::internal and trigger the request to intercept the request.
    std::string PreviewRequestBodyForTesting() { return BuildBody(); }
    std::string PreviewRequestHeadersForTesting() { return BuildHeaders(); }
    Builder& SetUserClassForTesting(const std::string& user_class) {
      user_class_ = user_class;
      return *this;
    }

   private:
    std::string BuildHeaders() const;
    std::string BuildBody() const;
    std::unique_ptr<net::URLFetcher> BuildURLFetcher(
        net::URLFetcherDelegate* request,
        const std::string& headers,
        const std::string& body) const;

    void PrepareLanguages(
        language::UrlLanguageHistogram::LanguageInfo* ui_language,
        language::UrlLanguageHistogram::LanguageInfo* other_top_language) const;

    // Only required, if the request needs to be sent.
    std::string auth_header_;
    base::Clock* clock_;
    RequestParams params_;
    ParseJSONCallback parse_json_callback_;
    GURL url_;
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

    // Optional properties.
    std::string obfuscated_gaia_id_;
    std::string user_class_;
    const language::UrlLanguageHistogram* language_histogram_;

    DISALLOW_COPY_AND_ASSIGN(Builder);
  };

  JsonRequest(base::Optional<Category> exclusive_category,
              base::Clock* clock,
              const ParseJSONCallback& callback);
  JsonRequest(JsonRequest&&);
  ~JsonRequest() override;

  void Start(CompletedCallback callback);

  const base::Optional<Category>& exclusive_category() const {
    return exclusive_category_;
  }

  base::TimeDelta GetFetchDuration() const;
  std::string GetResponseString() const;

 private:
  // URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void ParseJsonResponse();
  void OnJsonParsed(std::unique_ptr<base::Value> result);
  void OnJsonError(const std::string& error);

  // The fetcher for downloading the snippets. Only non-null if a fetch is
  // currently ongoing.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // If set, only return results for this category.
  base::Optional<Category> exclusive_category_;

  // Use the Clock from the Fetcher to measure the fetch time. It will be
  // used on creation and after the fetch returned. It has to be alive until the
  // request is destroyed.
  base::Clock* clock_;
  base::Time creation_time_;

  // This callback is called to parse a json string. It contains callbacks for
  // error and success cases.
  ParseJSONCallback parse_json_callback_;

  // The callback to notify when URLFetcher finished and results are available.
  CompletedCallback request_completed_callback_;

  base::WeakPtrFactory<JsonRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(JsonRequest);
};

}  // namespace internal

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_JSON_REQUEST_H_
