{-# OPTIONS_GHC -XScopedTypeVariables #-}
module Main (main) where
import Control.Monad (replicateM)
import Data.List (isInfixOf)
import Data.Maybe
import Data.Time
import GHC.IO.Handle
import Stats
import System.Environment (getArgs)
import System.Process (CreateProcess(..), createProcess, proc, waitForProcess, StdStream(..))
import Text.Printf


doBenchFPS :: Int -> String -> [String] -> IO ()
doBenchFPS repeats cmd args = do
  times_ <- replicateM repeats $ fpsCommand cmd args
  let times = catMaybes times_
  putStrLn $ "Frame times: " ++ (show times)
  printf "Min/med/max: %01.2f  %01.2f  %01.2f ms\n" (minimum times) (median times) (maximum times)
  printf "Mean: %01.2f  +/-  %01.2f ms\n" (mean times) (stddev times)


fpsCommand :: String -> [String] -> IO (Maybe Double)
fpsCommand cmd args = do
  -- no NoStream in the version I'm using currently...
  let c = (proc cmd args) { std_in = CreatePipe, std_out = CreatePipe, std_err = CreatePipe }
  
  (hIn, hOut, hErr, procHandle) <- createProcess c
  case hIn of
    Nothing -> return ()
    Just h  -> hClose h
  
  out <- case hOut of
    Just o -> do
     output <- hGetContents o
     let l = lines output
     let fpsLines = filter (isInfixOf " fps") l
     case fpsLines of
       []    -> return Nothing
       (s:_) -> do
         let w = words s
         case drop 4 w of
           (s:_) -> do let fps = read s
                       return $ Just (1000.0 / fps)
           _     -> do putStrLn "no FPS, check command"
                       return Nothing
  waitForProcess procHandle
  return out


main :: IO ()
main = do
  args <- getArgs
  case args of
   (n:c:as ) -> do let n_ = reads n
                   case n_ of
                     [] -> do putStrLn "Bad repeats"
                     ((repeats,_):_) -> doBenchFPS repeats c as
   _         -> do putStrLn "Usage: benchTime numRepeats command"


