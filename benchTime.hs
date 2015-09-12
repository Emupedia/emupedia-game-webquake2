{-# OPTIONS_GHC -XScopedTypeVariables #-}
module Main (main) where
import Control.Monad (replicateM)
import Data.Time
import GHC.IO.Handle
import Stats
import System.Environment (getArgs)
import System.Process (CreateProcess(..), createProcess, proc, waitForProcess, StdStream(..))
import Text.Printf


doBenchTime :: Int -> String -> [String] -> IO ()
doBenchTime repeats cmd args = do
  times_ <- replicateM repeats $ timeCommand cmd args
  putStrLn $ "Times: " ++ (show times_)
  let (times :: [Double]) = map realToFrac times_
  printf "Min/med/max: %01.2f  %01.2f  %01.2f sec\n" (minimum times) (median times) (maximum times)
  printf "Mean: %01.2f  +/-  %01.2f sec\n" (mean times) (stddev times)


timeCommand :: String -> [String] -> IO NominalDiffTime
timeCommand cmd args = do
  startTime <- getCurrentTime
  runCommand cmd args
  stopTime <- getCurrentTime
  return $ diffUTCTime stopTime startTime
  


runCommand :: String -> [String] -> IO ()
runCommand cmd args = do
  -- no NoStream in the version I'm using currently...
  let c = (proc cmd args) { std_in = CreatePipe, std_out = CreatePipe, std_err = CreatePipe }
  
  (hIn, hOut, hErr, procHandle) <- createProcess c
  case hIn of
    Nothing -> return ()
    Just h  -> hClose h
  case (hOut, hErr) of
   (Just o, Just e) -> outLoop o e
   _                -> return ()
  _ <- waitForProcess procHandle
  return ()
  where
    outLoop :: Handle -> Handle -> IO ()
    outLoop hOut hErr = do
      outClosed <- hIsClosed hOut
      errClosed <- hIsClosed hErr
      if outClosed && errClosed
        then return ()
        else do
          eofE <- hIsEOF hErr
          if not eofE
            then hGetLine hErr
            else return []
          eofO <- hIsEOF hOut
          if not eofO
            then do hGetLine hOut
                    outLoop hOut hErr
            else return ()
  


main :: IO ()
main = do
  args <- getArgs
  case args of
   (n:c:as ) -> do let n_ = reads n
                   case n_ of
                     [] -> do putStrLn "Bad repeats"
                     ((repeats,_):_) -> doBenchTime repeats c as
   _         -> do putStrLn "Usage: benchTime numRepeats command"


