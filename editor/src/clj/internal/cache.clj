(ns internal.cache
  (:require [clojure.core.async :as a]
            [clojure.core.cache :as cc]
            [internal.either :as e]))

(def ^:dynamic *cache-debug* nil)

(defn- build-leastness-queue
  [base limit start-at]
  (into (clojure.data.priority-map/priority-map)
        (concat (take (- limit (count base)) (for [k (range (- limit) 0)] [k k]))
                (for [[k _] base] [k start-at]))))

(defn- post-removal [ch v]
  (when v
    (cond
      (and (satisfies? e/Either v) (e/exists? v) (not (nil? (e/result v))))
      (a/>!! ch (e/result v))

      (not (satisfies? e/Either v))
      (a/>!! ch v)

      :else
      :ignored)))

;; This is the LRUCache from clojure.core.cache,
;; but with an added core.async channel that
;; removed values are put into
(cc/defcache LRUCache [cache lru tick limit ch]
  cc/CacheProtocol
  (lookup [_ item]
          (get cache item))
  (lookup [_ item not-found]
          (get cache item not-found))
  (has? [_ item]
        (contains? cache item))
  (hit [_ item]
       (let [tick+ (inc tick)]
         (LRUCache. cache
                    (if (contains? cache item)
                      (assoc lru item tick+)
                      lru)
                    tick+
                    limit
                    ch)))
  (miss [_ item result]
        (let [tick+ (inc tick)]
          (if (>= (count lru) limit)
            (let [k (if (contains? lru item)
                      item
                      (first (peek lru))) ;; minimum-key, maybe evict case
                  c (-> cache (dissoc k) (assoc item result))
                  l (-> lru (dissoc k) (assoc item tick+))]
              (when-not (= item k)
                (post-removal ch (get cache k)))
              (LRUCache. c l tick+ limit ch))
            (LRUCache. (assoc cache item result) ;; no change case
                       (assoc lru item tick+)
                       tick+
                       limit
                       ch))))
  (evict [this key]
         (let [v (get cache key ::miss)]
           (if (= v ::miss)
             this
             (do
               (post-removal ch v)
               (LRUCache. (dissoc cache key)
                          (dissoc lru key)
                          (inc tick)
                          limit
                          ch)))))
  (seed [_ base]
        (LRUCache. base
                   (build-leastness-queue base limit 0)
                   0
                   limit
                   ch))
  Object
  (toString [_]
            (str cache \, \space lru \, \space tick \, \space limit)))

(defn lru-cache-factory
  "Returns an LRU cache with the cache and usage-table initialied to `base` --
   each entry is initialized with the same usage value.
   This function takes an optional `:threshold` argument that defines the maximum number
   of elements in the cache before the LRU semantics apply (default is 32)."
  [base & {threshold :threshold dispose-ch :dispose-ch :or {threshold 32}}]
  {:pre [(number? threshold) (< 0 threshold)
         (map? base)]}
  (clojure.core.cache/seed (LRUCache. {} (clojure.data.priority-map/priority-map) 0 threshold dispose-ch) base))

(defn- encache
  [c kvs]
  (when *cache-debug*
    (println 'internal.cache/encache (map first kvs)))
  (reduce
   (fn [c [k v]]
     (cc/miss c k v))
   c kvs))

(defn- hits
  [c ks]
  (when *cache-debug*
    (println 'internal.cache/hits ks))
  (reduce
   (fn [c [k v]]
     (cc/hit c k))
   c ks))

(defn- evict
  [cache ks]
  (when *cache-debug*
    (println 'internal.cache/evict ks))
  (reduce cc/evict cache ks))

;; ----------------------------------------
;; Interface
;; ----------------------------------------
(def default-cache-limit 1000)

(defn cache-subsystem
  ([]
   (cache-subsystem default-cache-limit (a/chan (a/dropping-buffer 1))))
  ([limit dispose-ch]
   (atom (lru-cache-factory {} :threshold limit :dispose-ch dispose-ch))))

(defn cache-snapshot
  [cache]
  @cache)

(defn cache-hit
  [cache ks]
  (swap! cache hits ks))

(defn cache-encache
  [cache kvs]
  (swap! cache encache kvs))

(defn cache-invalidate
  [cache ks]
  (swap! cache evict ks))
