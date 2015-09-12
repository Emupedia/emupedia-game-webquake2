{-# OPTIONS_GHC -XBangPatterns #-}

module Stats where

-- copied from Math.Statistics

-----------------------------------------------------------------------------
-- Module      : Math.Statistics
-- Copyright   : (c) 2008 Marshall Beddoe
-- License     : BSD3
--
-- Maintainer  : mbeddoe@<nospam>gmail.com
-- Stability   : experimental
-- Portability : portable
--
-- Description :
--   A collection of commonly used statistical functions.
-----------------------------------------------------------------------------

import Data.List
import Data.Ord (comparing)

-- |Numerically stable mean
mean :: Floating a => [a] -> a
mean x = fst $ foldl' (\(!m, !n) x -> (m+(x-m)/(n+1),n+1)) (0,0) x


-- |Median
median :: (Floating a, Ord a) => [a] -> a
median x | odd n  = head  $ drop (n `div` 2) x'
         | even n = mean $ take 2 $ drop i x'
                  where i = (length x' `div` 2) - 1
                        x' = sort x
                        n  = length x


-- |Sample variance
var xs = (var' 0 0 0 xs) / (fromIntegral $ length xs - 1)
    where
      var' _ _ s [] = s
      var' m n s (x:xs) = var' nm (n + 1) (s + delta * (x - nm)) xs
         where
           delta = x - m
           nm = m + delta/(fromIntegral $ n + 1)



-- |Standard deviation of sample
stddev :: (Floating a) => [a] -> a
stddev xs = sqrt $ var xs


