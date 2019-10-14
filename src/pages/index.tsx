import classNames from 'classnames'
import React, { useReducer } from 'react'

import Dialog, {
  DialogActions,
  DialogContent,
  DialogTitle,
} from '../components/Dialog'
import Button from '../components/Button'
import FileUploader from '../components/FileUploader'
import VideoConverter from '../components/VideoConverter'
import NotificationToasts from '../components/NotificationToasts'

import styles from './index.module.scss'

const FILE_SIZE_LIMIT = 200 * 1024 * 1024 // 200 Mb

interface State {
  file: File | null
  errorTitle?: string
  errorMessage?: string
  errorDialogOpen: boolean
}

const defaultState: State = {
  file: null,
  errorTitle: undefined,
  errorMessage: undefined,
  errorDialogOpen: false,
}

type Action =
  | { type: 'set file'; file: File }
  | { type: 'reset' }
  | { type: 'set error'; title: string; message: string }
  | { type: 'dismiss error' }
  | { type: 'set filesize error' }

const reducer: React.Reducer<State, Action> = (state, action) => {
  switch (action.type) {
    case 'set file': {
      return {
        file: action.file,
        errorTitle: undefined,
        errorMessage: undefined,
        errorDialogOpen: false,
      }
    }
    case 'set error': {
      return {
        ...state,
        errorTitle: action.title,
        errorMessage: action.message,
        errorDialogOpen: true,
      }
    }
    case 'set filesize error': {
      return {
        ...state,
        errorTitle: 'File too large',
        errorMessage:
          'The size of the uploaded file exceeds our limit (200 Mb)',
        errorDialogOpen: true,
      }
    }
    case 'dismiss error': {
      return {
        ...state,
        errorTitle: undefined,
        errorMessage: undefined,
        errorDialogOpen: false,
      }
    }
    case 'reset': {
      return defaultState
    }
    default: {
      return state
    }
  }
}

const Index: React.FC = () => {
  const [
    { file, errorTitle, errorMessage, errorDialogOpen },
    dispatch,
  ] = useReducer(reducer, defaultState)

  const handleFile = async (inputFile: File) => {
    if (inputFile.size > FILE_SIZE_LIMIT) {
      dispatch({ type: 'set filesize error' })
      return
    }

    dispatch({ type: 'set file', file: inputFile })
  }

  const handleDialogClose = () => {
    dispatch({ type: 'dismiss error' })
  }

  return (
    <div className="flex flex-column min-vh-100">
      {file ? (
        <VideoConverter
          video={file}
          onClose={() => dispatch({ type: 'reset' })}
        />
      ) : (
        <>
          <FileUploader
            className={styles.fileUploader}
            onFile={handleFile}
            onError={message => {
              dispatch({ type: 'set error', title: 'Error', message })
            }}
          />
          <Dialog
            open={errorDialogOpen}
            onClose={handleDialogClose}
            role="alertdialog"
          >
            <DialogTitle>{errorTitle}</DialogTitle>
            <DialogContent>{errorMessage}</DialogContent>
            <DialogActions>
              <Button onClick={handleDialogClose}>Close</Button>
            </DialogActions>
          </Dialog>
          <footer className="w-100 pv3">
            <ul
              className={classNames(
                styles.footerList,
                'flex justify-center list'
              )}
            >
              <li>
                <a
                  className="link underline-hover"
                  href="https://github.com/lucasecdb/convideo"
                  rel="noopener noreferrer"
                  target="_blank"
                >
                  View the code
                </a>
              </li>
              <li>
                <a
                  className="link underline-hover"
                  href="https://github.com/lucasecdb/convideo/issues"
                  rel="noopener noreferrer"
                  target="_blank"
                >
                  Report a bug
                </a>
              </li>
            </ul>
          </footer>
        </>
      )}
      <NotificationToasts />
    </div>
  )
}

export default Index
